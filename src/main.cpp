#include <iostream>

#include "engine/ecs/World.hpp"


class ChildOf {
};


struct Player : Required<ChildOf> {
};


int main() {
    ecs::World world;

    const ecs::Entity player = world.entity();
}
