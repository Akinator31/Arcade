#pragma once
#include "Query.hpp"
#include "internal/ArchetypeRegistry.hpp"
#include "internal/EntityRegistry.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <utility>
#include <tuple>

template<typename Func, typename Tuple, size_t... I>
static inline void call(Func &func, Tuple &rows, uint i, std::index_sequence<I...>) {
    func(std::get<I>(rows)[i]...);
}

struct Phase {
    std::vector<std::pair<ecs::QueryID, void (*)(ArchetypeView &)> > systems;
};

using PhaseId = uint32_t;
using SystemId = std::tuple<PhaseId, uint32_t>;


template<typename... Components>
struct All {
    template<typename Func>
    static void each(Func func) {
        (func(reflection::type_id<Components>()), ...);
    }
};

template<typename System>
concept IsSystem = requires(System system, ArchetypeView &view)
{
    {
        system.iter(view)
    };
};

template<typename System>
concept HasRequired = requires(System system)
{
    {
        System::with::each([](ecs::ComponentID) {
        })
    };
};

template<typename System>
concept HasExcluded = requires(System system)
{
    {
        System::without::each([](ecs::ComponentID) {
        })
    };
};

namespace ecs {
    class World {
        internal::EntityRegistry entity_registry;
        internal::ComponentRegistry component_registry;
        internal::ArchetypeRegistry archetype_registry;
        std::vector<QueryCache> queries;
        std::vector<Phase> phases;

    public:
        World();

        Entity entity() { return this->entity_registry.create(); }

        void kill(const Entity entity) {
            return this->entity_registry.destroy(entity);
        }

        [[nodiscard]] bool isAlive(const Entity entity) {
            return this->entity_registry.isAlive(entity);
        }

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

        PhaseId createPhase() {
            this->phases.emplace({});
            return this->phases.size() - 1;
        }

        template<IsSystem System>
        SystemId registerSystem(PhaseId phase) {
            Query query;
            if constexpr (HasRequired<System>) {
                System::with::each([&query](ComponentID cid) {
                    query.require(cid);
                });
            }
            if constexpr (HasExcluded<System>) {
                System::without::each([&query](ComponentID cid) {
                    query.exclude(cid);
                });
            }
            QueryID qid = this->cache(std::move(query));
            this->phases[phase].systems.push_back({qid, System::iter});
            return {phase, this->phases[phase].systems.size() - 1};
        }

        void runSystem(SystemId sys) {
            auto [phase, index] = sys;
            auto [qid, callback] = this->phases[phase].systems.at(index);
            this->read(qid).iter(callback);
        }

        const std::vector<internal::Archetype> &getArchetypes() const;

        std::vector<internal::Archetype> &getArchetypes();

        QueryID cache(Query &&q) {
            QueryCache cached(std::move(q));

            this->updateMatches(cached);
            this->queries.push_back(std::move(cached));
            return this->queries.size() - 1;
        }

        const datastructures::EcsVec<ArchetypeID> &matches(const QueryID qid) const {
            return this->queries.at(qid).matches;
        }

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
                for (const ecs::ArchetypeID tableId: *this->matches) {
                    auto view = ArchetypeView(this->world->getArchetypes().at(tableId), *this->world);
                    func(view);
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
            QueryCache cached(std::move(query<Components...>()));
            this->updateMatches(cached);
            return OwnedTablesReader(std::move(cached), *this);
        }

        TablesReader read(const QueryID qid) {
            return {this->queries.at(qid).matches, *this};
        }

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
