#pragma once
#include "Query.hpp"
#include "internal/ArchetypeRegistry.hpp"
#include "internal/EntityRegistry.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <utility>
#include <tuple>

#include "Component.hpp"
#include "State.hpp"
#include "System.hpp"
#include "Timer.hpp"
#include "engine/reflection/TypeCounter.hpp"
#include "internal/EventRegistry.hpp"
#include "Relation.hpp"


template<typename T>
struct RelationSource {
    datastructures::EcsVec<ecs::Entity> entities;
};

template<typename T>
struct RelationTarget {
    ecs::Entity target;
};

class Hierarchy {
};

using Parent = RelationTarget<Hierarchy>;
using Children = RelationSource<Hierarchy>;


namespace ecs {
    class World;


    struct PluginFamily {
    };

    using PluginId = uint16_t;

    template<typename T>
    concept IsPlugin = requires(T plugin, World &world)
    {
        { plugin.load(world) };
        { plugin.unload(world) };
    };

    template<typename T>
    concept HasOnAdd = requires(World &world, Entity entity)
    {
        { T::onAdd(world, entity) };
    };
    template<typename T>
    concept HasOnSet = requires(World &world, Entity entity, const T *value)
    {
        { T::onSet(world, entity, value) };
    };

    class EntityRef;

    class World : public internal::EventRegistry, public StateRegistry {
        struct PluginRecord {
            void *instance = nullptr;

            void (*unload)(void *, World &) = nullptr;

            void (*destroy)(void *) = nullptr;
        };

        internal::EntityRegistry entity_registry;
        internal::ArchetypeRegistry archetype_registry;
        std::vector<QueryCache> queries;
        PhaseContainer phase_container;
        std::vector<Phase> phases;
        std::vector<PluginRecord> loaded_plugins;

    public:
        internal::ComponentRegistry component_registry;
        float deltaTime{};

        World();

        ~World();

        template<typename T, typename... Args>
        void plugin(Args &&... args) {
            const PluginId id = reflection::TypeCounter<PluginFamily>::template id<T>();
            if (id >= this->loaded_plugins.size()) {
                this->loaded_plugins.resize(id + 1);
            }
            if (this->loaded_plugins[id].instance == nullptr) {
                T *instance = new T(std::forward<Args>(args)...);
                instance->load(*this);
                this->loaded_plugins[id] = {
                    instance,
                    [](void *ptr, World &w) { static_cast<T *>(ptr)->unload(w); },
                    [](void *ptr) { delete static_cast<T *>(ptr); }
                };
            }
        }

        template<typename T>
        void removePlugin() {
            if (const PluginId id = reflection::TypeCounter<PluginFamily>::template id<T>();
                id < this->loaded_plugins.size() && this->loaded_plugins[id].instance != nullptr) {
                this->loaded_plugins[id].unload(this->loaded_plugins[id].instance, *this);
                this->loaded_plugins[id].destroy(this->loaded_plugins[id].instance);
                this->loaded_plugins[id] = {nullptr, nullptr, nullptr};
            }
        }

        template<typename T>
        bool hasPlugin() const {
            if (const PluginId id = reflection::TypeCounter<PluginFamily>::template id<T>();
                id < this->loaded_plugins.size()) {
                return this->loaded_plugins[id].instance != nullptr;
            }
            return false;
        }

        template<typename T>
        T *getPlugin() const {
            if (const PluginId id = reflection::TypeCounter<PluginFamily>::template id<T>();
                id < this->loaded_plugins.size() && this->loaded_plugins[id].instance != nullptr) {
                return static_cast<T *>(this->loaded_plugins[id].instance);
            }
            return nullptr;
        }

        EntityRef entity();

        void kill(Entity entity);

        [[nodiscard]] bool isAlive(Entity entity);

        template<typename T>
        void add(const Entity entity) {
            this->component_registry.registerComponent<T>();

            if (this->add_id(entity, reflection::type_id<T>())) {
                if constexpr (HasOnAdd<T>) {
                    T::onAdd(*this, entity);
                }
                if constexpr (HasFromWorldConstructor<T>) {
                    T *value = this->get<T>(entity);
                    *value = T(*this);
                } else if constexpr (HasDefaultConstructor<T>) {
                    T *value = this->get<T>(entity);
                    *value = T();
                }
                if constexpr (HasRequiredComponents<T>) {
                    T::add(*this, entity);
                }
            }
        }

        template<typename T>
        void remove(const Entity entity) {
            this->remove_id(entity, reflection::type_id<T>());
        }

        template<typename T>
        T *get(const Entity entity) {
            return static_cast<T *>(this->get_id(entity, reflection::type_id<T>()));
        }


        template<typename T>
        bool has(const Entity entity) {
            return this->archetype_registry.getArchetype(this->entity_registry.getRecord(entity).archetypeId).has(
                reflection::type_id<T>());
        }

        template<typename T>
        void set(const Entity entity, const T &value) {
            this->add<T>(entity);
            T *current = this->get<T>(entity);
            memcpy(current, &value, sizeof(T));
            if constexpr (HasOnSet<T>) {
                T::onSet(*this, entity, current);
            }
        }

        template<typename T>
        void set(const Entity entity, const T &&value) {
            this->add<T>(entity);
            T *current = this->get<T>(entity);
            *current = std::move(value);
            if constexpr (HasOnSet<T>) {
                T::onSet(*this, entity, current);
            }
        }

    private:
        template<typename T>
        bool remove_target(const Entity source) {
            using Target = RelationTarget<T>;
            using Source = RelationSource<T>;
            if (this->has<Target>(source)) {
                Target *oldTarget = this->get<Target>(source);
                datastructures::EcsVec<Entity> &entities = this->get<Source>(oldTarget->target)->entities;
                entities.remove(source);
                if (entities.size == 0) {
                    this->remove<Source>(oldTarget->target);
                }
                return true;
            }
            return false;
        }

        template<typename T>
        void add_target(const Entity source, const Entity target) {
            using Source = RelationSource<T>;

            this->add<Source>(target);
            this->get<Source>(target)->entities.push_back(source);
        }

    public:
        template<typename T>
        void relation() {
            using Target = RelationTarget<T>;
            using Source = RelationSource<T>;

            struct OnDespawnTarget : With<Target>, On<Despawn> {
                static void observe(internal::Archetype &arch, internal::EntityRow row) {
                    arch.world.remove_target<T>(arch.getEntities()[row]);
                }
            };

            struct OnDespawnSource : With<Source>, On<Despawn> {
                static void observe(internal::Archetype &arch, internal::EntityRow row) {
                    for (auto *sources = static_cast<Source *>(arch.getComponent(row, reflection::type_id<Source>()));
                         const Entity entity: sources->entities) {
                        arch.world.unrelate<T>(entity);
                    }
                }
            };

            this->system<OnDespawnTarget>();
            this->system<OnDespawnSource>();
        }

        template<typename T>
        void relate(const Entity source, const Entity target) {
            using Target = RelationTarget<T>;

            this->remove_target<T>(source);

            this->set<Target>(source, {.target = target});

            this->add_target<T>(source, target);
        }

        template<typename T>
        bool has_target(const Entity source, const Entity target) {
            using Target = RelationTarget<T>;

            if (this->has<Target>(source) && this->get<Target>(source)->target == target) {
                return true;
            }
            return false;
        }

        template<typename T>
        bool has_source(const Entity source, const Entity target) {
            using Source = RelationSource<T>;

            if (this->has<Source>(source) && this->get<Source>(source)->entities.has(target)) {
                return true;
            }
            return false;
        }

        template<typename T, bool Recursive = false>
        auto iterRelated(const Entity target) {
            ComponentID source_id = reflection::type_id<RelationSource<T> >();
            if constexpr (Recursive) {
                return RelatedRangeRecursive{this, target, source_id};
            } else {
                return RelatedRangeNonRecursive{this, target, source_id};
            }
        }

        template<typename T>
        void unrelate(const Entity source) {
            using Target = RelationTarget<T>;

            this->remove_target<T>(source);
            this->remove<Target>(source);
        }

        template<typename Event>
        void emit(const Entity entity, Event evt) {
            if (const uint64_t id = this->id<Event>(entity); this->entity_event_map.contains(id)) {
                auto *sys = static_cast<EntityEvent<Event> *>(this->entity_event_map.at(id));
                sys->callback(*this, entity, evt);
            }
        }

        template<typename Phase>
        PhaseId phase() {
            const PhaseId id = this->phase_container.phase<Phase>();
            if (id >= this->phases.size()) {
                this->phases.resize(id + 1);
            }
            return id;
        }

        template<typename System>
        SystemId system() {
            PhaseId pid = this->phase<Update>();

            if constexpr (HasPhase<System>) {
                pid = this->phase<typename System::phase>();
            }

            Query query;

            if constexpr (HasRequired<System>) {
                System::with::require(query);
            }
            if constexpr (HasExcluded<System>) {
                System::without::exclude(query);
            }

            SystemRegistered config = {
                SystemCounter::id<System>(),
                this->cache(std::move(query)),
                nullptr, new System(), nullptr,
                getSystemCondition<System>()
            };

            if constexpr (IsObserver<System>) {
                auto attach = [](World &world, const ArchetypeID id) {
                    auto &arch = world.archetype_registry.getArchetype(id);

                    if constexpr (std::is_same_v<typename System::phase, Despawn>)
                        arch.onDespawn.push_back(System::observe);
                    else if constexpr (std::is_same_v<typename System::phase, Remove>)
                        System::add_removed_components(arch, System::observe);
                    else
                        arch.onAdd.push_back(System::observe);
                };

                this->queries.at(config.qid).on_add = attach;

                for (const ArchetypeID tid: this->queries.at(config.qid).matches)
                    attach(*this, tid);

                return {0, 0};
            }

            if constexpr (IsSystem<System>) {
                config.iter = System::iter;
            }
            if constexpr (Runnable<System>) {
                config.run = reinterpret_cast<void(*)(void *, World &)>(System::run);
            }
            this->phases[pid].systems.push_back(config);
            return {pid, this->phases[pid].systems.size() - 1};
        }

        template<typename System>
        void remove() {
            static_assert(IsSystem<System> || IsObserver<System> || Runnable<System>, "system not valid");
            for (auto &[systems]: this->phases) {
                auto it = systems.begin();
                while (it != systems.end()) {
                    if (it->id == SystemCounter::id<System>()) {
                        it = systems.erase(it);
                    } else {
                        it += 1;
                    }
                }
            }
        }

        void runSystem(SystemId sys);

        const std::vector<internal::Archetype> &getArchetypes() const;

        std::vector<internal::Archetype> &getArchetypes();

        QueryID cache(Query &&q);

        const datastructures::EcsVec<ArchetypeID> &matches(QueryID qid) const;

        template<typename... Filter>
        static bool filter(ArchetypeView &view, internal::EntityRow row) {
            return (Filter{}(view, row) && ...);
        }

        class TablesReader {
        protected:
            const datastructures::EcsVec<ArchetypeID> *matches = nullptr;
            World *world = nullptr;

        public:
            TablesReader() = default;

            TablesReader(const datastructures::EcsVec<ArchetypeID> &matches, World &world)
                : matches(&matches), world(&world) {
            }

            template<typename Func>
                requires std::invocable<Func, ArchetypeView &>
            void iter(Func &&func) {
                for (const ArchetypeID tableId: *this->matches) {
                    if (auto view = ArchetypeView(this->world->getArchetypes().at(tableId), *this->world);
                        view.count() > 0) {
                        func(view);
                    }
                }
            }
        };

        class OwnedTablesReader : public TablesReader {
            QueryCache cache;

        public:
            explicit OwnedTablesReader(QueryCache &&cache, World &world) : TablesReader(),
                                                                           cache(std::move(cache)) {
                this->matches = &this->cache.matches;
                this->world = &world;
            }
        };

        template
        <
            typename
            ...
            Components>

        auto fetch() {
            QueryCache cached;
            cached.required<Components...>();
            this->updateMatches(cached);
            return OwnedTablesReader(std::move(cached), *this);
        }

        TablesReader read(QueryID qid);

        const std::vector<SystemRegistered> &getSystems(PhaseId phase);

        void runAll(PhaseId pid);

        void progress();

        void start();

    public:
        void *get_id(Entity entity, ComponentID cid);

    private:
        void removeEntityOfArchetype(internal::Archetype &oldArch,
                                     internal::EntityRow row);

        bool add_id(Entity entity, ComponentID cid);

        void remove_id(Entity entity, ComponentID cid);

        void migrate(Entity, ArchetypeID newArchId);

        void updateMatches(QueryCache &cached);

        ArchetypeID findOrCreateArchetype(EntityType &&type);
    };

    class EntityRef : public Entity {
        World &world;

    public:
        explicit EntityRef(World &world, const Entity entity) : Entity(entity), world(world) {
        }

        template<typename... Components>
        EntityRef &&add() {
            (this->world.add<Components>(this), ...);
            return std::move(*this);
        }


        template<typename... Components>
        EntityRef &&set(Components... value) {
            (this->world.set<Components>(this->entity(), value), ...);
            return std::move(*this);
        }

        [[nodiscard]] Entity entity() const {
            return {this->index, this->generation};
        }
    };
} // namespace ecs

template<typename... Components>
struct Required {
    static void add(ecs::World &world, const ecs::Entity entity) {
        (world.add<Components>(entity), ...);
    }
};

template<int>
struct Interval {
    static Timer timer;

    static bool condition(ecs::World &world) {
        return timer.tick(world.deltaTime);
    }
};

template<int value>
Timer Interval<value>::timer = Timer(value / 1000);

template<auto value>
struct InState {
    static bool condition(ecs::World &world) {
        return world.getState<decltype(value)>() == value;
    }
};
