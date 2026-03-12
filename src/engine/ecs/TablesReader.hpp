#pragma once

#include "Query.hpp"
#include "internal/Archetype.hpp"

namespace ecs {
    class World;

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
        void iter(Func &&func);
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
}
