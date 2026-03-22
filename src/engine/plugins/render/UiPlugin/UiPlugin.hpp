#pragma once
#include <cmath>
#include <cstring>
#include <string_view>
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

struct AnimatedSprite : Required<Sprite, HoveredSensorComponent> {
    Sprite base;
    Sprite hover;
    Sprite click;
    bool enabled;

    AnimatedSprite(const Sprite& base, const Sprite& hover, const Sprite& click, const bool enabled = true) :
        Required(), base(base), hover(hover), click(click), enabled(enabled) {}
};

struct ImageOnHoverComponent : Required<HoveredSensorComponent> {
    Sprite image;
    Sprite imageOnHover;

    ImageOnHoverComponent(const Sprite& image, const Sprite& imageOnHover) :
        Required{}, image(image), imageOnHover(imageOnHover) {}
};

struct Text {
    ResourceIndex font;
    const char* text;
    Color color;
    uint32_t fontSize;
    IVec2 offset;
};

struct TextOnSpriteComponent : Required<Sprite> {
    static constexpr size_t MAX_TEXT_LEN = 256;

    ResourceIndex font;
    char text[MAX_TEXT_LEN];
    Color color;
    uint32_t fontSize;
    IVec2 offset;

    TextOnSpriteComponent(const ResourceIndex font, const std::string& value, const Color color,
                          const uint32_t fontSize = 16, const IVec2 offset = {0, 0}) :
        Required(), font(font), text{}, color(color), fontSize(fontSize), offset(offset) {
        std::strncpy(this->text, value.c_str(), MAX_TEXT_LEN - 1);
        this->text[MAX_TEXT_LEN - 1] = '\0';
    }
};

struct ClickedEvent {};

struct MouseEnterEvent {};

struct MouseExitEvent {};

bool mouseInRect(const IVec2& mousePos, const Position& position, const Size& size);

SYSTEM(TextOnSpriteSys, On<PreUpdate>, With<TextOnSpriteComponent>) {
    ITER(view) {
        const auto textOnSprites = view.column<TextOnSpriteComponent>();

        for (uint i = 0; i < view.count(); i++) {
            view.world.set(
                view.entity(i),
                Text{
                    .font = textOnSprites[i].font,
                    .text = textOnSprites[i].text,
                    .color = textOnSprites[i].color,
                    .fontSize = textOnSprites[i].fontSize,
                    .offset = textOnSprites[i].offset
                }
            );
        }
    }
};

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

SYSTEM(AnimatedSpriteSys, With<AnimatedSprite, Sprite>, On<PostUpdate>) {
    ITER(view) {
        auto* animatedSprites = view.column<AnimatedSprite>();
        auto* sprites = view.column<Sprite>();

        for (uint i = 0; i < view.count(); i++) {
            if (!animatedSprites[i].enabled) {
                sprites[i] = animatedSprites[i].base;
                continue;
            }

            const ecs::Entity entity = view.entity(i);

            if (view.world.has<PressedComponent>(entity)) {
                sprites[i] = animatedSprites[i].click;
            } else if (view.world.has<HoveredComponent>(entity)) {
                sprites[i] = animatedSprites[i].hover;
            } else {
                sprites[i] = animatedSprites[i].base;
            }
        }
    }
};

struct UiPlugin {
    void load(ecs::World& world) {
        world.state(MouseButtonLeftState::NONE);
        world.registerComponent<Text>();
        world.registerComponent<HoveredComponent>();
        world.registerComponent<HoveredSensorComponent>();
        world.registerComponent<ImageOnHoverComponent>();
        world.registerComponent<TrackMouseOnPressedComponent>();
        world.registerComponent<PressedComponent>();
        world.registerComponent<TextOnSpriteComponent>();
        world.registerComponent<AnimatedSprite>();
        world.system<MouseButtonLeftSys>();
        world.system<TextOnSpriteSys>();
        world.system<HoveredSys>();
        world.system<ImageOnHoverSys>();
        world.system<UnHoveredButtonSys>();
        world.system<EntityClickedSys>();
        world.system<TrackMouseOnPressedSys>();
        world.system<PressedSys>();
        world.system<AnimatedSpriteSys>();
    }

    void unload(ecs::World& world) {
        world.remove<MouseButtonLeftSys>();
        world.remove<TextOnSpriteSys>();
        world.remove<HoveredSys>();
        world.remove<EntityClickedSys>();
        world.remove<TrackMouseOnPressedSys>();
        world.remove<PressedSys>();
        world.remove<AnimatedSpriteSys>();
        world.remove<ImageOnHoverSys>();
        world.remove<UnHoveredButtonSys>();
    }
};