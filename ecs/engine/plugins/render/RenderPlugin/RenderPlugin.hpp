#pragma once
#include "engine/ecs/World.hpp"
#include "engine/Graphics.hpp"
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
        auto *api = view.world.singleton_get<GraphicsApi>();

        const auto func = get_virtual<void (*)(void *, GlobalPosition, Size, Color)>(api, 19);

        for (uint i = 0; i < view.count(); i++) {
            func(api, positions[i], sizes[i], colors[i]);
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
