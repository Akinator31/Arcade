#pragma once
#include "engine/ecs/World.hpp"

enum class MouseButtonLeftState {
    NONE,
    RELEASED,
    PRESSED,
};

struct HoveredComponent : Required<Position, Size> {};
struct HoveredSensorComponent : Required<Position, Size> {};

struct ClickedEvent {};
struct MouseEnterEvent {};
struct MouseExitEvent {};

bool mouseInRect(const IVec2 &mousePos, const Position &position, const Size &size);

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

SYSTEM(HoveredSys, With<HoveredSensorComponent>, On<PostUpdate>) {
    ITER(view) {
        const auto *positions = view.column<Position>();
        const auto *sizes = view.column<Size>();
        const bool hasHoveredComponent = view.optional<HoveredComponent>();
        const auto mousePos = view.world.api->getMousePosition();

        for (uint i = 0; i < view.count(); i++) {
            if (mouseInRect(mousePos, positions[i], sizes[i])) {
                if (!hasHoveredComponent) {
                    view.world.command([entity = view.entity(i)](ecs::World &world) {
                        world.add<HoveredComponent>(entity);
                        world.emit(entity, MouseEnterEvent{});
                    });
                }
            } else {
                view.world.command([entity = view.entity(i)](ecs::World &world) {
                        world.remove<HoveredComponent>(entity);
                        world.emit(entity, MouseExitEvent{});
                    });
            }
        }
  }
};

SYSTEM(EntityClickedSys, With<HoveredComponent>, On<PostUpdate>, InState<MouseButtonLeftState::RELEASED>) {
  ITER(view) {
      for (uint i = 0; i < view.count(); i++) {
          view.world.emit(view.entity(i), ClickedEvent{});
      }
  }
};

struct UiPlugin {
    void load(ecs::World &world) {
        world.state(MouseButtonLeftState::NONE);
        world.registerComponent<HoveredComponent>();
        world.registerComponent<HoveredSensorComponent>();
        world.system<MouseButtonLeftSys>();
        world.system<HoveredSys>();
        world.system<EntityClickedSys>();
    }

    void unload(ecs::World &world) {
        world.remove<MouseButtonLeftSys>();
        world.remove<HoveredSys>();
    }
};