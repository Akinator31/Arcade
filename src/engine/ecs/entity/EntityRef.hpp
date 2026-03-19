#pragma once

#include "type.hpp"
#include "engine/ecs/World.hpp"
#include "engine/ecs/World.hpp"

namespace ecs {
    class World;

    class EntityRef : public Entity {
    public:
        World &world;

        explicit EntityRef(World &world, const Entity entity) : Entity(entity), world(world) {
        }

        template<class... Components>
        explicit EntityRef(World &tempWorld, Components... all);

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

        template<class... Components>
        static EntityRef make(World &world, Components... all);


        [[nodiscard]] Entity entity() const {
            return {this->index, this->generation};
        }

        [[nodiscard]] Entity id() const {
            return {this->index, this->generation};
        }
    };
}
