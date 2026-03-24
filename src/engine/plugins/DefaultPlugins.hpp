#pragma once
#include "cli/CliPlugin.hpp"
#include "control/ControlPlugin.hpp"
#include "engine/ecs/World.hpp"
#include "render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"
#include "render/RenderPlugin/RenderPlugin.hpp"
#include "sprite/SpritePlugin.hpp"
#include "physics/PhysicsPlugin.hpp"
#include "render/UiPlugin/UiPlugin.hpp"

struct DefaultPlugin {
    void load(ecs::World& world) {
        world.plugin<PositionPropagationPlugin>();
        world.plugin<RenderPlugin>();
        world.plugin<PhysicsPlugin>();
        world.plugin<ControlPlugin>();
        world.plugin<CliPlugin>(CliMode::Server, 4040);
        world.plugin<UiPlugin>();
        world.plugin<SpritePlugin>();
    }

    void unload(ecs::World&) {
        // le world fait déjà un le unload ???
    }
};

struct MenuScenePlugin {
    void load(ecs::World& world) {
        world.plugin<PositionPropagationPlugin>();
        world.plugin<RenderPlugin>();
        world.plugin<PhysicsPlugin>();
        world.plugin<ControlPlugin>();
        world.plugin<UiPlugin>();
        world.plugin<SpritePlugin>();
    }

    void unload(ecs::World&) {
        // le world fait déjà un le unload ???
    }
};