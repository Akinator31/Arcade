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

namespace ecs {
    class World {
        internal::EntityRegistry entity_registry;
        internal::ComponentRegistry component_registry;
        internal::ArchetypeRegistry archetype_registry;
        std::vector<QueryCache> queries;
        PhaseContainer phase_container;
        std::vector<Phase> phases;

    public:
        World();

        Entity entity();

        void kill(Entity entity);

        [[nodiscard]] bool isAlive(Entity entity);

        template<typename T>
        void add(const Entity entity) {
            this->component_registry.registerComponent<T>();
            this->add_id(entity, reflection::type_id<T>());
        }

        template<typename T>
        void remove(const Entity entity) {
            this->remove_id(entity, reflection::type_id<T>());
        }

        template<typename T>
        T *get(const Entity entity) {
            return static_cast<T *>(this->get_id(entity, reflection::type_id<T>()));
        }

        template<typename Phase>
        PhaseId phase() {
            PhaseId id = this->phase_container.phase<Phase>();
            if (id >= this->phases.size()) {
                this->phases.resize(id + 1);
            }
            return id;
        }

        template<IsSystem System, typename Phase>
        SystemId registerSystem() {
            PhaseId phase_id = this->phase<Phase>();
            Query query;
            if constexpr (HasRequired<System>) {
                System::with::require(query);
            }
            if constexpr (HasExcluded<System>) {
                System::without::exclude(query);
            }
            QueryID qid = this->cache(std::move(query));
            this->phases[phase_id].systems.push_back({qid, System::iter});

            return {phase_id, this->phases[phase_id].systems.size() - 1};
        }

        void runSystem(SystemId sys);

        const std::vector<internal::Archetype> &getArchetypes() const;

        std::vector<internal::Archetype> &getArchetypes();

        QueryID cache(Query &&q);

        const datastructures::EcsVec<ArchetypeID> &matches(const QueryID qid) const;

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

        void add_id(Entity entity, ComponentID cid);

        void remove_id(Entity entity, ComponentID cid);

        void *get_id(Entity entity, ComponentID cid);

        void migrate(Entity, ArchetypeID newArchId);

        void updateMatches(QueryCache &cached);

        ArchetypeID findOrCreateArchetype(EntityType &&type);
    };
} // namespace ecs
