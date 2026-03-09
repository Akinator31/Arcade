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


namespace ecs {
    class World {
        internal::EntityRegistry entity_registry;
        internal::ComponentRegistry component_registry;
        internal::ArchetypeRegistry archetype_registry;
        std::vector<QueryCache> queries;

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

        const std::vector<internal::Archetype> &getArchetypes() const;

        std::vector<internal::Archetype> &getArchetypes();

        template<typename... Components>
        QueryID cache(Query<Components...> &&q) {
            QueryCache cached(q.raw());

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

        template<typename... Components>
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
            QueryCache cached(std::move(query<Components...>().raw()));
            this->updateMatches(cached);
            return OwnedTablesReader<Components...>(std::move(cached), *this);
        }

        auto read(const QueryID qid) {
            return TablesReader(this->queries.at(qid).matches, *this);
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
