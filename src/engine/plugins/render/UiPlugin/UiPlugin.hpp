#pragma once
#include "engine/ecs/World.hpp"

enum class MouseButtonLeftState {
    NONE,
    RELEASED,
    PRESSED,
};

struct HoveredComponent : Required<Position, Size> {};

SYSTEM(MouseButtonLeftSys, On<PreUpdate>) {
    static void run(ecs::World& world) {
        const auto *graphicsApi = world.api;

        world.state(MouseButtonLeftState::NONE);

        if (graphicsApi->wasMouseButtonReleased(0)) {
            world.state(MouseButtonLeftState::RELEASED);
        } else if (graphicsApi->isMouseButtonPressed(0)) {
            world.state(MouseButtonLeftState::PRESSED);
        }
    }
};

struct UiPlugin {
    void load(ecs::World &world) {
        world.state(MouseButtonLeftState::NONE);
        world.registerComponent<HoveredComponent>();
        world.system<MouseButtonLeftSys>();
    }

    void unload([[maybe_unused]]ecs::World &world) {
        world.remove<MouseButtonLeftSys>();
    }
};