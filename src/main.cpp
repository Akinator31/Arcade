#include <iostream>

#include "engine/ecs/World.hpp"


class ChildOf {
};

struct Player {
};


struct MySystem : With<Player>, On<Add> {
    static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
        puts("ok");
    }
};

struct Damaged {
    int value;
};


int main() {
    ecs::World world;

    const ecs::Entity player = world.entity();

    world.listen<Damaged>(player, [](auto &, auto, auto) {
        puts("player damaged");
    });
}
