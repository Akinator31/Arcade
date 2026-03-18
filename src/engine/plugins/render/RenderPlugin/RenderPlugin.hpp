#pragma once
#include "engine/ecs/World.hpp"
#include "engine/plugins/render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"

template<class Fn>
Fn get_virtual(void *obj, const size_t index) {
    void **vtable = *static_cast<void ***>(obj);
    return reinterpret_cast<Fn>(vtable[index]);
}

SYSTEM(RenderRectSys, With<GlobalPosition, Size, Color>) {
    ITER(view) {
        const auto *positions = view.column<GlobalPosition>();
        const auto *sizes = view.column<Size>();
        const auto *colors = view.column<Color>();

        for (uint i = 0; i < view.count(); i++) {
            view.world.api->drawRect(positions[i], sizes[i], colors[i]);
        }
    }
};

SYSTEM(RenderSpriteSys, With<GlobalPosition, Sprite>, On<Render>) {
    ITER(view) {
        const auto *positions = view.column<GlobalPosition>();
        const auto *sprites = view.column<Sprite>();

        for (uint i = 0; i < view.count(); i++) {
            view.world.api->drawSprite(positions[i], sprites[i]);
        }
    }
};


struct RenderPlugin {
    void load(ecs::World &world) {
        world.plugin<PositionPropagationPlugin>();
        world.registerComponent<Size>();
        world.registerComponent<Color>();
        world.system<RenderRectSys>();
    }

    void unload(ecs::World &world) {
        world.remove<RenderRectSys>();
    }
};
