#pragma once
#include "cli/CliPlugin.hpp"
#include "engine/ecs/World.hpp"
#include "physics/PhysicsPlugin.hpp"
#include "render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"
#include "render/RenderPlugin/RenderPlugin.hpp"
#include "sprite/SpritePlugin.hpp"

struct DefaultPlugin {
    void load(ecs::World &world) {
        world.plugin<PositionPropagationPlugin>();
        world.plugin<PhysicsPlugin>();
        world.plugin<RenderPlugin>();
        world.plugin<SpritePlugin>();
        world.plugin<CliPlugin>(CliMode::Server, 4040);
    }

    void unload(ecs::World &world) {
        world.removePlugin<PositionPropagationPlugin>();
        world.removePlugin<RenderPlugin>();
        world.plugin<CliPlugin>();
    }
};
