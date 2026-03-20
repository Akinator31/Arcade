#pragma once

#include "engine/ecs/World.hpp"
#include "engine/plugins/render/CameraPlugin/CameraPlugin.hpp"
#include "engine/plugins/render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"

template<class Fn>
Fn get_virtual(void *obj, const size_t index) {
    void **vtable = *static_cast<void ***>(obj);
    return reinterpret_cast<Fn>(vtable[index]);
}

namespace render_plugin_impl {
    inline GlobalPosition worldToCamera(const GlobalPosition &world_position, const GlobalPosition &camera_position) {
        return {
            world_position.x - camera_position.x,
            world_position.y - camera_position.y
        };
    }
}

SYSTEM(RenderRectSys, With<GlobalPosition, Size, Color>, On<Render>) {
    ITER(view) {
        const ecs::Entity main_camera = camera_plugin_impl::mainCamera(view.world);
        const GlobalPosition main_camera_position = *view.world.get<GlobalPosition>(main_camera);
        const auto *positions = view.column<GlobalPosition>();
        const auto *sizes = view.column<Size>();
        const auto *colors = view.column<Color>();
        const auto *targets = view.optional<CameraTarget>();

        for (uint i = 0; i < view.count(); i++) {
            const GlobalPosition camera_position = targets == nullptr
                                                       ? main_camera_position
                                                       : *view.world.get<GlobalPosition>(targets[i].camera);
            view.world.api->drawRect(
                render_plugin_impl::worldToCamera(positions[i], camera_position),
                sizes[i],
                colors[i]
            );
        }
    }
};

SYSTEM(RenderSpriteSys, With<GlobalPosition, Sprite>, On<Render>) {
    ITER(view) {
        const ecs::Entity main_camera = camera_plugin_impl::mainCamera(view.world);
        const GlobalPosition main_camera_position = *view.world.get<GlobalPosition>(main_camera);
        const auto *positions = view.column<GlobalPosition>();
        const auto *sprites = view.column<Sprite>();
        const auto *targets = view.optional<CameraTarget>();

        for (uint i = 0; i < view.count(); i++) {
            const GlobalPosition camera_position = targets == nullptr
                                                       ? main_camera_position
                                                       : *view.world.get<GlobalPosition>(targets[i].camera);
            view.world.api->drawSprite(
                render_plugin_impl::worldToCamera(positions[i], camera_position),
                sprites[i]
            );
        }
    }
};


struct RenderPlugin {
    void load(ecs::World &world) {
        world.plugin<CameraPlugin>();
        world.registerComponent<Size>();
        world.registerComponent<Color>();
        world.system<RenderRectSys>();
        world.system<RenderSpriteSys>();
    }

    void unload(ecs::World &world) {
        world.remove<RenderRectSys>();
        world.remove<RenderSpriteSys>();
    }
};
