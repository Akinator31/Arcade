#pragma once
#include "engine/ecs/World.hpp"

enum class MouseButtonLeftState {
    NONE,
    RELEASED,
    PRESSED,
};

struct HoveredComponent : Required<Position, Size> {};

struct HoveredSensorComponent : Required<Position, Size> {};

struct PressedComponent : Required<Position, Size> {};

struct TrackMouseOnPressedComponent : Required<HoveredComponent> {};

struct ImageOnHoverComponent : Required<HoveredSensorComponent> {
    Sprite image;
    Sprite imageOnHover;

    ImageOnHoverComponent(const Sprite& image, const Sprite& imageOnHover) :
        Required<HoveredSensorComponent>{}, image(image), imageOnHover(imageOnHover) {}
};

struct ClickedEvent {};

struct MouseEnterEvent {};

struct MouseExitEvent {};

bool mouseInRect(const IVec2& mousePos, const Position& position, const Size& size);

SYSTEM(MouseButtonLeftSys, On<PreUpdate>) {
    static void run(ecs::World& world) {
        const auto* graphicsApi = world.api;

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
        const auto* positions = view.column<Position>();
        const auto* sizes = view.column<Size>();
        const bool hasHoveredComponent = view.optional<HoveredComponent>();
        const auto mousePos = view.world.api->getMousePosition();

        for (uint i = 0; i < view.count(); i++) {
            if (mouseInRect(mousePos, positions[i], sizes[i])) {
                if (!hasHoveredComponent) {
                    view.world.command([entity = view.entity(i)](ecs::World& world) {
                        world.add<HoveredComponent>(entity);
                        world.emit(entity, MouseEnterEvent{});
                    });
                    return;
                }
            } else {
                if (view.world.has<HoveredComponent>(view.entity(i))) {
                    view.world.remove<HoveredComponent>(view.entity(i));
                    view.world.emit(view.entity(i), MouseExitEvent{});
                }
            }
        }
    }
};

SYSTEM(ImageOnHoverSys, On<Add>, With<HoveredComponent, ImageOnHoverComponent, Sprite>) {
    OBSERVE(table, row) {
        auto* sprite = static_cast<Sprite*>(table.getComponent(row, reflection::type_id<Sprite>()));
        const auto* hover = static_cast<ImageOnHoverComponent*>(table.getComponent(
            row, reflection::type_id<ImageOnHoverComponent>()));

        *sprite = hover->imageOnHover;
    }
};

SYSTEM(UnHoveredButtonSys, On<Remove>, With<HoveredComponent, ImageOnHoverComponent, Sprite>) {
    OBSERVE(table, row) {
        auto* sprite = static_cast<Sprite*>(table.getComponent(row, reflection::type_id<Sprite>()));
        const auto* hover = static_cast<ImageOnHoverComponent*>(table.getComponent(
            row, reflection::type_id<ImageOnHoverComponent>()));

        *sprite = hover->image;
    }
};

SYSTEM(PressedSys, With<Position, Size>, On<PostUpdate>) {
    ITER(view) {
        const auto isMousePressed = view.world.getState<MouseButtonLeftState>();
        const auto mousePos = view.world.api->getMousePosition();
        const auto* positions = view.column<Position>();
        const auto* sizes = view.column<Size>();

        for (uint i = 0; i < view.count(); i++) {
            if (isMousePressed != MouseButtonLeftState::PRESSED) {
                view.world.command([entity = view.entity(i)](ecs::World& world) {
                    world.remove<PressedComponent>(entity);
                });
                return;
            }
            if (mouseInRect(mousePos, positions[i], sizes[i])) {
                view.world.command([entity = view.entity(i)](ecs::World& world) {
                    world.add<PressedComponent>(entity);
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

SYSTEM(TrackMouseOnPressedSys, With<TrackMouseOnPressedComponent, PressedComponent>, On<PostUpdate>) {
    ITER(view) {
        const auto [x, y] = view.world.api->getMousePosition();
        const auto* sizes = view.column<Size>();
        auto* positions = view.column<Position>();

        for (uint i = 0; i < view.count(); i++) {
            positions[i] = Position{
                static_cast<float>(x) - sizes[i].width / 2, static_cast<float>(y) - sizes[i].height / 2
            };
        }
    }
};

struct UiPlugin {
    void load(ecs::World& world) {
        world.state(MouseButtonLeftState::NONE);
        world.registerComponent<HoveredComponent>();
        world.registerComponent<HoveredSensorComponent>();
        world.registerComponent<ImageOnHoverComponent>();
        world.registerComponent<TrackMouseOnPressedComponent>();
        world.registerComponent<PressedComponent>();
        world.system<MouseButtonLeftSys>();
        world.system<HoveredSys>();
        world.system<ImageOnHoverSys>();
        world.system<UnHoveredButtonSys>();
        world.system<EntityClickedSys>();
        world.system<TrackMouseOnPressedSys>();
        world.system<PressedSys>();
    }

    void unload(ecs::World& world) {
        world.remove<MouseButtonLeftSys>();
        world.remove<HoveredSys>();
        world.system<EntityClickedSys>();
        world.remove<TrackMouseOnPressedSys>();
        world.remove<PressedSys>();
        world.remove<ImageOnHoverSys>();
    }
};