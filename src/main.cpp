#include <iostream>

#include "engine/ecs/World.hpp"


struct Player : Required<ChildOf> {
};


int main() {
    ecs::World world;

    const ecs::Entity parent = world.entity();
    const ecs::Entity child = world.entity();

    world.relate<ChildOf>(child, parent);
}
