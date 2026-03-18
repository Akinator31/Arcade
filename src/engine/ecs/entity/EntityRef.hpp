#pragma once

#include "type.hpp"
#include "../World.hpp"

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

        [[nodiscard]] EntityRef child() const;

        template<typename Relation>
        EntityRef &&relate(Entity target);

        template<typename Event, typename Func>
        EntityRef &&listen(Func &&func);

        template<typename T>
        T *get();

        [[nodiscard]] Entity entity() const {
            return {this->index, this->generation};
        }

        [[nodiscard]] Entity id() const {
            return {this->index, this->generation};
        }
    };
}