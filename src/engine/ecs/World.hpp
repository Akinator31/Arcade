#pragma once
#include <functional>

#include "system/Query.hpp"
#include "internal/Registry/ArchetypeRegistry.hpp"
#include "internal/Registry/EntityRegistry.hpp"
#include "internal/Registry/ComponentRegistry.hpp"
#include "components/Component.hpp"
#include "addons/State.hpp"
#include "system/System.hpp"
#include "addons/Timer.hpp"
#include "engine/reflection/TypeCounter.hpp"
#include "internal/Registry/EventRegistry.hpp"
#include "internal/Registry/GlobalEventRegistry.hpp"
#include "components/Relation.hpp"
#include "addons/Singleton.hpp"
#include "components/CoreComponents.hpp"
#include "entity/EntityRef.hpp"
#include "Plugin.hpp"
#include "IDisplayModule.hpp"
#include "utils/TablesReader.hpp"

namespace ecs {
    class World : public internal::EventRegistry,
                  public internal::GlobalEventRegistry,
                  public StateRegistry,
                  public SingletonRegistry {
        PhaseContainer phase_container;
        std::vector<Phase> phases;
        std::vector<PluginRecord> loaded_plugins;
        std::unordered_map<std::string, Entity> entity_name_to_entity;
        std::vector<std::function<void (World &)> > commands;

    public:
        std::vector<QueryCache> queries;
        internal::EntityRegistry entity_registry;
        internal::ComponentRegistry component_registry;
        internal::ArchetypeRegistry archetype_registry;
        IDisplayModule *api = nullptr;

        float deltaTime{};

        World();

        ~World() override;

        template<typename T, typename... Args>
        void plugin(Args &&... args) {
            const PluginId id = reflection::TypeCounter<PluginFamily>::id<T>();
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
            if (const PluginId id = reflection::TypeCounter<PluginFamily>::id<T>();
                id < this->loaded_plugins.size() && this->loaded_plugins[id].instance != nullptr) {
                this->loaded_plugins[id].unload(this->loaded_plugins[id].instance, *this);
                this->loaded_plugins[id].destroy(this->loaded_plugins[id].instance);
                this->loaded_plugins[id] = {nullptr, nullptr, nullptr};
            }
        }

        template<typename T>
        bool hasPlugin() const {
            if (const PluginId id = reflection::TypeCounter<PluginFamily>::id<T>();
                id < this->loaded_plugins.size()) {
                return this->loaded_plugins[id].instance != nullptr;
            }
            return false;
        }

        template<typename T>
        T *getPlugin() const {
            if (const PluginId id = reflection::TypeCounter<PluginFamily>::id<T>();
                id < this->loaded_plugins.size() && this->loaded_plugins[id].instance != nullptr) {
                return static_cast<T *>(this->loaded_plugins[id].instance);
            }
            return nullptr;
        }

        template<HasConstruct T>
        EntityRef create(T::Props props) {
            EntityRef ref = this->create();
            if constexpr (HasConstruct<T>) {
                T::construct(ref, props);
            }
            return ref;
        }

        template<HasConstruct T>
        EntityRef create() {
            return create<T>(T::Default());
        }

        Entity entity();

        Entity clone(Entity entity);

        EntityRef create();

        void kill(Entity entity);

        [[nodiscard]] bool isAlive(Entity entity);

        static std::string makeEntityName(Entity entity);

        void syncEntityName(Entity entity);

        void clearEntityName(Entity entity);

        [[nodiscard]] std::optional<Entity> findEntityByName(const std::string &name) const;

        void flush();

        void command(const std::function<void (World &)> &);

        template<typename T>
        void remove(const Entity entity) {
            this->remove_id(entity, reflection::type_id<T>());
        }

        template<typename T>
        void registerComponent() {
            const ComponentID cid = reflection::type_id<T>();
            if (this->component_registry.isRegistered(cid)) {
                return;
            }

            this->component_registry.registerComponent<T>();
            auto &record = this->component_registry.getRecord(cid);

            if constexpr (reflection::is_complete<T>()) {
                if constexpr (HasFromWorldConstructor<T>) {
                    record.construct = [](World &world, void *ptr) {
                        if (ptr != nullptr) {
                            *static_cast<T *>(ptr) = T(world);
                        }
                    };
                } else if constexpr (HasDefaultConstructor<T>) {
                    record.construct = [](World &, void *ptr) {
                        if (ptr != nullptr) {
                            *static_cast<T *>(ptr) = T();
                        }
                    };
                }
            }

            if constexpr (HasOnAdd<T>) {
                record.onAdd = [](World &world, const Entity entity) {
                    T::onAdd(world, entity);
                };
            }
            if constexpr (HasOnRemove<T>) {
                record.onRemove = [](World &world, const Entity entity, const void *value) {
                    T::onRemove(world, entity, static_cast<const T *>(value));
                };
            }

            if constexpr (HasRequiredComponents<T>) {
                for (const ComponentID required_cid: T::required_components()) {
                    this->component_registry.addRequired(cid, required_cid);
                }
            }
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
            const ComponentID cid = reflection::type_id<T>();
            this->add_id(entity, cid);
            T *current = this->get<T>(entity);
            if constexpr (std::is_same_v<T, Name>) {
                this->clearEntityName(entity);
            }
            this->runOnRemove(entity, cid, current);
            memcpy(current, &value, sizeof(T));
            if constexpr (HasOnSet<T>) {
                T::onSet(*this, entity, current);
            }
        }

        template<typename T>
        void set(const Entity entity, const T &&value) {
            const ComponentID cid = reflection::type_id<T>();
            this->add_id(entity, cid);
            T *current = this->get<T>(entity);
            if constexpr (std::is_same_v<T, Name>) {
                this->clearEntityName(entity);
            }
            this->runOnRemove(entity, cid, current);
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
                auto *oldTarget = this->get<Target>(source);
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

            this->registerComponent<Target>();
            this->registerComponent<Source>();

            struct OnDespawnTarget : With<Target>, On<Despawn> {
                // ReSharper disable once CppParameterMayBeConstPtrOrRef
                static void observe(internal::Archetype &arch, const internal::EntityRow row) {
                    arch.world.remove_target<T>(arch.getEntities()[row]);
                }
            };

            struct OnDespawnSource : With<Source>, On<Despawn> {
                // ReSharper disable once CppParameterMayBeConstPtrOrRef
                static void observe(internal::Archetype &arch, const internal::EntityRow row) {
                    auto *sources = static_cast<Source *>(arch.getComponent(row, reflection::type_id<Source>()));
                    for (auto entities_copy = sources->entities.clone(); const Entity entity: entities_copy) {
                        if constexpr (requires { requires T::despawn_related; }) {
                            arch.world.kill(entity);
                        } else {
                            arch.world.unrelate<T>(entity);
                        }
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
            const ComponentID source_id = reflection::type_id<RelationSource<T> >();
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
                auto *sys = static_cast<EntityEvent<Event> *>(this->entity_event_map.at(id).instance);
                const auto listeners = sys->listeners;
                for (const auto &listener: listeners) {
                    listener.callback(*this, entity, evt);
                }
            }
        }

        template<typename Event, typename Func>
            requires std::invocable<Func, World &, Entity, const Event>
        EventListenerId listen(const Entity entity, Func &&func) {
            return EventRegistry::listen<Event>(entity, std::forward<Func>(func));
        }

        template<typename Event>
        void unlisten(const Entity entity) {
            internal::EventRegistry::unlisten<Event>(entity);
        }

        template<typename Event>
        void unlisten(const Entity entity, const EventListenerId listener_id) {
            internal::EventRegistry::unlisten<Event>(entity, listener_id);
        }

        template<typename Event>
        void globalEmit(Event evt) {
            internal::GlobalEventRegistry::emit<Event>(*this, evt);
        }

        template<typename Event, typename Func>
            requires std::invocable<Func, World &, const Event>
        EventListenerId globalListen(Func &&func) {
            return internal::GlobalEventRegistry::listen<Event>(std::forward<Func>(func));
        }

        template<typename Event, typename Func>
            requires std::invocable<Func, World &, const Event>
        EventListenerId globalListen(const Entity owner, Func &&func) {
            return internal::GlobalEventRegistry::listen<Event>(owner, std::forward<Func>(func));
        }

        template<typename Event>
        void globalUnlisten() {
            internal::GlobalEventRegistry::unlisten<Event>();
        }

        template<typename Event>
        void globalUnlisten(const EventListenerId listener_id) {
            internal::GlobalEventRegistry::unlisten<Event>(listener_id);
        }

        template<typename Event>
        void globalUnlisten(const Entity entity, const EventListenerId listener_id) {
            internal::GlobalEventRegistry::unlisten<Event>(entity, listener_id);
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

            System *sys = nullptr;

            if constexpr (HasFromWorldConstructor<System>) {
                sys = new System(*this);
            } else if constexpr (HasDefaultConstructor<System>) {
                sys = new System();
            }

            SystemRegistered config = {
                SystemCounter::id<System>(),
                this->cache(std::move(query)),
                nullptr, sys, nullptr,
                [](void *ptr) { delete static_cast<System *>(ptr); },
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

                if (config.destroy) {
                    config.destroy(config.value);
                    config.value = nullptr;
                    config.destroy = nullptr;
                }
                this->phases[pid].systems.push_back(config);
                return {pid, this->phases[pid].systems.size() - 1};
            }

            if constexpr (IsSystem<System>) {
                config.iter = System::iter;
            }
            if constexpr (MemberRunnable<System>) {
                static_assert(HasFromWorldConstructor<System> || HasDefaultConstructor<System>,
                              "member runnable systems must be default constructible or constructible from ecs::World");
                config.run = [](void *ptr, World &world) {
                    static_cast<System *>(ptr)->run(world);
                };
            } else if constexpr (StaticRunnable<System>) {
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
                        if constexpr (IsObserver<System>) {
                            QueryCache &query = this->queries.at(it->qid);

                            for (const ArchetypeID tid: query.matches) {
                                auto &arch = this->archetype_registry.getArchetype(tid);

                                if constexpr (std::is_same_v<typename System::phase, Despawn>) {
                                    arch.onDespawn.remove(System::observe);
                                } else if constexpr (std::is_same_v<typename System::phase, Remove>) {
                                    System::remove_removed_components(arch, System::observe);
                                } else {
                                    arch.onAdd.remove(System::observe);
                                }
                            }

                            query.on_add = nullptr;
                        }
                        if (it->destroy) {
                            it->destroy(it->value);
                        }
                        it = systems.erase(it);
                    } else {
                        it += 1;
                    }
                }
            }
        }

        void runSystem(const SystemId &sys);

        const std::vector<internal::Archetype> &getArchetypes() const;

        std::vector<internal::Archetype> &getArchetypes();

        QueryID cache(Query &&q);

        const datastructures::EcsVec<ArchetypeID> &matches(QueryID qid) const;


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

        void *get_id(Entity entity, ComponentID cid);

        void add_id(Entity entity, ComponentID cid);

        void remove_id(Entity entity, ComponentID cid);

        bool add_id_batched(Entity entity, const ComponentID *cid, uint count);

        template<typename... Components>
        void add(const Entity entity) {
            const ComponentID cid[] = {reflection::type_id<Components>()...};

            if constexpr (sizeof...(Components) > 1) {
                this->add_id_batched(entity, cid, sizeof...(Components));
            } else {
                this->add_id(entity, cid[0]);
            }
        }

    private:
        void constructComponent(Entity entity, ComponentID cid);

        void runOnAdd(Entity entity, ComponentID cid);

        void runArchetypeOnAdd(Entity entity);

        void runOnRemove(Entity entity, ComponentID cid, const void *value);

        void removeEntityOfArchetype(internal::Archetype &oldArch,
                                     internal::EntityRow row);


        void migrate(Entity, ArchetypeID newArchId);

        void updateMatches(QueryCache &cached);

        ArchetypeID findOrCreateArchetype(EntityType &&type);
    };

    template<typename Func>
        requires std::invocable<Func, ArchetypeView &>
    void TablesReader::iter(Func &&func) {
        for (const ArchetypeID tableId: *this->matches) {
            if (auto view = ArchetypeView(this->world->getArchetypes().at(tableId), *this->world);
                view.count() > 0) {
                func(view);
            }
        }
    }
} // namespace ecs

template<int>
struct Interval {
    static Timer timer;

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
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

namespace ecs {
    template<class... Components>
    EntityRef::EntityRef(World &tempWorld, Components... all) : Entity(tempWorld.create()), world(tempWorld) {
        this->set(all...);
    }

    template<typename... Components>
    EntityRef &&EntityRef::add() {
        world.add<Components...>(this->entity());
        return std::move(*this);
    }

    template<typename Relation>
    EntityRef &&EntityRef::relate(const Entity target) {
        world.relate<Relation>(this->entity(), target);
        return std::move(*this);
    }

    template<typename... Components>
    EntityRef &&EntityRef::set(Components... value) {
        (this->world.set<Components>(this->entity(), value), ...);
        return std::move(*this);
    }

    inline EntityRef EntityRef::child() const {
        return this->world.create().relate<Hierarchy>(this->entity());
    }

    inline void EntityRef::childOf(const Entity entity) const {
        this->world.relate<Hierarchy>(entity, this->entity());
    }

    template<typename Event, typename Func>
    EntityRef &&EntityRef::listen(Func &&func) {
        this->world.listen<Event>(this->entity(), func);
        return std::move(*this);
    }

    template<typename Event, typename Func>
    EntityRef &&EntityRef::globalListen(Func &&func) {
        this->world.globalListen<Event>(this->entity(), func);
        return std::move(*this);
    }

    template<typename T>
    T *EntityRef::get() {
        return this->world.get<T>(this->entity());
    }

    template<typename... Components>
    EntityRef EntityRef::make(World &world, Components... all) {
        const EntityRef ref = world.create().set(all...);
        return ref;
    }
}
