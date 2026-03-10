#pragma once
#include "Query.hpp"
#include "internal/ArchetypeRegistry.hpp"
#include "internal/EntityRegistry.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <utility>
#include <tuple>
#include "System.hpp"
#include "engine/reflection/TypeCounter.hpp"
#include "internal/EventRegistry.hpp"

namespace ecs {
    class World;

    namespace relation {
        template<typename T>
        struct RelationSource {
            datastructures::EcsVec<ecs::Entity> entities;
        };

        template<typename T>
        struct RelationTarget {
            ecs::Entity target;

            static void onSet(World &world, Entity entity, const RelationTarget<T> *value);
        };
    }

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


    class World : public internal::EventRegistry {
        struct PluginRecord {
            void *instance = nullptr;

            void (*unload)(void *, World &) = nullptr;

            void (*destroy)(void *) = nullptr;
        };

        internal::EntityRegistry entity_registry;
        internal::ComponentRegistry component_registry;
        internal::ArchetypeRegistry archetype_registry;
        std::vector<QueryCache> queries;
        PhaseContainer phase_container;
        std::vector<Phase> phases;
        std::vector<PluginRecord> loaded_plugins;

    public:
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

        Entity entity();

        void kill(Entity entity);

        [[nodiscard]] bool isAlive(Entity entity);

        template<typename T>
        void add(const Entity entity) {
            this->component_registry.registerComponent<T>();
            const bool is_added = this->add_id(entity, reflection::type_id<T>());
            if constexpr (HasOnAdd<T>) {
                if (is_added) {
                    T::onAdd(*this, entity);
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
        void set(const Entity entity, const T &value) {
            T *current = this->get<T>(entity);
            memcpy(current, &value, sizeof(T));
            if constexpr (HasOnSet<T>) {
                T::onSet(*this, entity, current);
            }
        }

        template<typename T>
        void set(const Entity entity, const T &&value) {
            T *current = this->get<T>(entity);
            *current = value;
        }

        template<typename T>
        void relate(Entity source, Entity target) {
            this->set<relation::RelationTarget<T> >(source, {target});
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
            PhaseId phase_id = this->phase<Update>();
            Query query;
            if constexpr (HasRequired<System>) {
                System::with::require(query);
            }
            if constexpr (HasExcluded<System>) {
                System::without::exclude(query);
            }

            if constexpr (HasPhase<System>) {
                phase_id = this->phase<typename System::phase>();
            }
            QueryID qid = this->cache(std::move(query));

            if constexpr (IsObserver<System>) {
                this->queries.at(qid).on_add = [](World &world, const ArchetypeID id) {
                    if constexpr (std::is_same<typename System::phase, Despawn>()) {
                        world.archetype_registry.getArchetype(id).onDespawn.push_back(System::observe);
                    } else if constexpr (IsOnRemove<typename System::phase>) {
                        System::phase::add(world.archetype_registry.getArchetype(id), System::observe);
                    } else {
                        world.archetype_registry.getArchetype(id).onAdd.push_back(System::observe);
                    }
                };
                for (const ArchetypeID &tid: this->queries.at(qid).matches) {
                    internal::Archetype &arch = this->archetype_registry.getArchetype(tid);

                    if constexpr (std::is_same<typename System::phase, Despawn>()) {
                        arch.onDespawn.push_back(System::observe);
                    } else if constexpr (IsOnRemove<typename System::phase>) {
                        System::phase::add(arch, System::observe);
                    } else {
                        arch.onAdd.push_back(System::observe);
                    }
                }
            } else if constexpr (IsSystem<System>) {
                this->phases[phase_id].systems.push_back({qid, System::iter});
                return {phase_id, this->phases[phase_id].systems.size() - 1};
            } else {
                static_assert(false, "system is not valid");
            }


            return {0, 0};
        }

        template<IsSystem System>
        void remove() {
            auto target_iter = System::iter;
            for (auto &[systems]: this->phases) {
                auto it = systems.begin();
                while (it != systems.end()) {
                    if (it->second == target_iter) {
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

        template<typename... Components>
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

    private:
        void removeEntityOfArchetype(internal::Archetype &oldArch,
                                     internal::EntityRow row);

        bool add_id(Entity entity, ComponentID cid);

        void remove_id(Entity entity, ComponentID cid);

        void *get_id(Entity entity, ComponentID cid);

        void migrate(Entity, ArchetypeID newArchId);

        void updateMatches(QueryCache &cached);

        ArchetypeID findOrCreateArchetype(EntityType &&type);
    };

    template<typename T>
    void relation::RelationTarget<T>::onSet(
        World &world,
        Entity entity,
        const RelationTarget<T> *value
    ) {
        world.add<RelationSource<T> >(value->target);
        RelationSource<T> *source = world.get<RelationSource<T> >(value->target);
        source->entities.push_back(entity);
    }
} // namespace ecs
