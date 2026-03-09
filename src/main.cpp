#include <iostream>

#include "engine/ecs/World.hpp"


struct Position {
    float x, y;
};

struct Velocity {
    float x, y;
};


struct Player {
};


struct MySystem : With<Position, Velocity>, On<Update> {
    static void iter(ArchetypeView &view) {
        auto *positions = view.column<Position>();
        const auto *velocities = view.column<Velocity>();

        for (uint i = 0; i < view.count(); i++) {
            positions[i].x += velocities[i].x;
            positions[i].y += velocities[i].y;
        }
    }
};


int main() {
    ecs::World world;

    world.system<MySystem>();

    world.progress();
}
