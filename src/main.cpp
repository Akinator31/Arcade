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


int main() {
    ecs::World world;


    world.system<MySystem>();

    world.add<Player>(world.entity());
}
