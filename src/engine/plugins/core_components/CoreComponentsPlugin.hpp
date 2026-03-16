#pragma once

#include "engine/ecs/World.hpp"

struct CoreComponentsPlugin {
    void load(ecs::World &world) {
        world.registerComponent<Name>();
        world.relation<Hierarchy>();
    }

    void unload(ecs::World &) {
    }
};
