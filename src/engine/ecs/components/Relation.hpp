#pragma once

#include <utility>
#include "engine/ecs/entity/type.hpp"
#include "engine/datastructures/EcsVec.hpp"

namespace ecs {
    class World;

    struct RelatedIteratorNonRecursive {
        Entity parent;
        Entity *current;

        std::pair<Entity, Entity> operator*() const;

        RelatedIteratorNonRecursive &operator++();

        bool operator!=(const RelatedIteratorNonRecursive &other) const;
    };

    struct RelatedIteratorRecursive {
        World *world;
        ComponentID source_id;

        struct Level {
            Entity parent;
            Entity *current;
            Entity *end;
        };

        Level stack[64];
        uint32_t stack_size = 0;

        RelatedIteratorRecursive(World *w, Entity start, ComponentID source_id);

        RelatedIteratorRecursive();

        std::pair<Entity, Entity> operator*() const;

        RelatedIteratorRecursive &operator++();

        bool operator!=(const RelatedIteratorRecursive &other) const;
    };

    struct RelatedRangeNonRecursive {
        World *world;
        Entity target;
        ComponentID source_id;

        [[nodiscard]] RelatedIteratorNonRecursive begin() const;

        [[nodiscard]] RelatedIteratorNonRecursive end() const;
    };

    struct RelatedRangeRecursive {
        World *world;
        Entity target;
        ComponentID source_id;

        [[nodiscard]] RelatedIteratorRecursive begin() const;

        [[nodiscard]] RelatedIteratorRecursive end() const;
    };

    struct DespawnRelated {
        static constexpr bool despawn_related = true;
    };
}