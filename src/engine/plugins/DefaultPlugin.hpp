#pragma once
#include "cli/CliPlugin.hpp"
#include "control/ControlPlugin.hpp"
#include "engine/ecs/World.hpp"
#include "render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"
#include "render/RenderPlugin/RenderPlugin.hpp"
#include "sprite/SpritePlugin.hpp"
#include "physics/PhysicsPlugin.hpp"
#include "render/UiPlugin/UiPlugin.hpp"
#include "tilemap/TileMapPlugin.hpp"

struct DefaultPlugin {
    void load(ecs::World &world) {
        world.plugin<PositionPropagationPlugin>();
        world.plugin<RenderPlugin>();
        world.plugin<PhysicsPlugin>();
        world.plugin<ControlPlugin>();
        world.plugin<UiPlugin>();
        world.plugin<SpritePlugin>();
        world.plugin<TileMapPlugin>();
    }

    void unload(ecs::World &) {
    }
};

struct MenuScenePlugin {
    void load(ecs::World &world) {
        world.plugin<PositionPropagationPlugin>();
        world.plugin<RenderPlugin>();
        world.plugin<PhysicsPlugin>();
        world.plugin<ControlPlugin>();
        world.plugin<UiPlugin>();
        world.plugin<SpritePlugin>();
    }

    void unload(ecs::World &) {
    }
};
