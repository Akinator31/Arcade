#pragma once

#include "type.hpp"

namespace ecs {
    class World;

    class EntityRef : public Entity {
        World &world;

    public:
        explicit EntityRef(World &world, const Entity entity) : Entity(entity), world(world) {
        }

        template<typename... Components>
        EntityRef &&add();


        template<typename... Components>
        EntityRef &&set(Components... value);

        [[nodiscard]] Entity entity() const {
            return {this->index, this->generation};
        }
    };
}
