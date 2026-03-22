#pragma once

#include "engine/ecs/World.hpp"
#include "engine/plugins/render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"

struct Camera : Required<Position, GlobalPosition> {
};

struct MainCamera : Required<Camera> {
};

struct CameraTarget {
    ecs::Entity camera;


    fields(
        field(uint32_t, "index")
        field(uint32_t, "generation")
    )
};

struct CameraFollow {
    ecs::Entity target{};
    GlobalPosition offset = {0.f, 0.f};
    bool centered = true;

    fields(
        field(uint32_t, "index")
        field(uint32_t, "generation")
        field(float, "offset_x")
        field(float, "offset_y")
        field(bool, "centered")
    )
};

namespace camera_plugin_impl {
    inline ecs::Entity mainCamera(ecs::World &world) {
        ecs::Entity main_camera{};
        world.fetch<MainCamera>().iter([&main_camera](ArchetypeView &view) {
            if (view.count() > 0) {
                main_camera = view.entity(0);
            }
        });
        return main_camera;
    }

    inline GlobalPosition worldPosition(ecs::World &world, const ecs::Entity entity) {
        const auto *position = world.get<Position>(entity);
        if (position == nullptr) {
            if (const auto *global_position = world.get<GlobalPosition>(entity); global_position != nullptr) {
                return *global_position;
            }
            return {};
        }

        GlobalPosition global_position{position->x, position->y};
        if (const auto *parent = world.get<Parent>(entity); parent != nullptr) {
            const GlobalPosition parent_position = worldPosition(world, parent->target);
            global_position.x += parent_position.x;
            global_position.y += parent_position.y;
        }
        return global_position;
    }
}

SYSTEM(CameraFollowSys, With<Camera, Position, CameraFollow>, On<PostUpdate>) {
    ITER(view) {
        auto *camera_positions = view.column<Position>();
        const auto *follows = view.column<CameraFollow>();
        const USize window_size = view.world.api == nullptr ? USize{0, 0} : view.world.api->getWindowSize();

        for (uint32_t i = 0; i < view.count(); i++) {
            const GlobalPosition target_position = camera_plugin_impl::worldPosition(view.world, follows[i].target);
            const float centered_x = follows[i].centered ? static_cast<float>(window_size.width) * 0.5f : 0.f;
            const float centered_y = follows[i].centered ? static_cast<float>(window_size.height) * 0.5f : 0.f;

            camera_positions[i].x = target_position.x - centered_x + follows[i].offset.x;
            camera_positions[i].y = target_position.y - centered_y + follows[i].offset.y;
        }
    }
};

struct CameraPlugin {
    void load(ecs::World &world) {
        world.plugin<PositionPropagationPlugin>();
        world.registerComponent<Camera>();
        world.registerComponent<MainCamera>();
        world.registerComponent<CameraTarget>();
        world.registerComponent<CameraFollow>();
        world.system<CameraFollowSys>();

        bool has_main_camera = false;
        world.fetch<MainCamera>().iter([&has_main_camera](ArchetypeView &view) {
            has_main_camera = view.count() > 0;
        });

        if (!has_main_camera) {
            world.create()
                    .set(Name{"MainCamera"}, Position{0.f, 0.f}, GlobalPosition{0.f, 0.f})
                    .add<MainCamera>();
        }
    }

    void unload(ecs::World &world) {
        world.remove<CameraFollowSys>();
    }
};
