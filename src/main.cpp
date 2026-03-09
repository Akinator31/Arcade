#include <iostream>
#include <ostream>

#include "engine/ecs/World.hpp"


struct Position {
};

struct Velocity {
};


struct Player {
};


struct MySystem {
    using with = All<Position>;
    using without = All<Player>;

    static void iter([[maybe_unused]] ArchetypeView &view) {
        puts("ok");
    }
};

int main() {
    ecs::World world;

    const PhaseId Startup = world.createPhase();

    const SystemId sys = world.registerSystem<MySystem>(Startup);

    world.add<Position>(world.entity());

    world.runSystem(sys);
}
