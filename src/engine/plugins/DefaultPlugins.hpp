#pragma once
#include "cli/CliPlugin.hpp"
#include "engine/ecs/World.hpp"
#include "render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"
#include "render/RenderPlugin/RenderPlugin.hpp"
#include "render/UiPlugin/UiPlugin.hpp"

struct DefaultPlugin {
    void load(ecs::World &world) {
        world.plugin<PositionPropagationPlugin>();
        world.plugin<RenderPlugin>();
        world.plugin<CliPlugin>(CliMode::Server, 4040);
        world.plugin<UiPlugin>();
    }

    void unload(ecs::World &world) {
        world.removePlugin<PositionPropagationPlugin>();
        world.removePlugin<RenderPlugin>();
        world.removePlugin<UiPlugin>();
        world.plugin<CliPlugin>();
    }
};