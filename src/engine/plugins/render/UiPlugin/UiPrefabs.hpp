#pragma once
#include <cmath>
#include "UiPlugin.hpp"
#include "Types.hpp"
#include "ecs/World.hpp"

struct Button {
    static constexpr ResourceIndex LARGE_BUTTON_TEXTURE = 0;
    static constexpr ResourceIndex LARGE_BUTTON_HOVER_TEXTURE = 1;
    static constexpr ResourceIndex LARGE_BUTTON_CLICK_TEXTURE = 2;

    struct Props {
        Position pos;
        float scale;
        bool animated;
    };

    static Props Default() {
        return Props{
            .pos = {0, 0},
            .scale = 1.f,
            .animated = true
        };
    }

    static void construct(ecs::EntityRef& ref, const Props& props) {
        const Sprite sprite{
            .texture = LARGE_BUTTON_TEXTURE,
            .scale = {props.scale, props.scale},
            .rect = {
                .left = 0,
                .top = 0,
                .width = 96,
                .height = 32,
            }
        };

        const Sprite spriteOnHover{
            .texture = LARGE_BUTTON_HOVER_TEXTURE,
            .scale = {props.scale, props.scale},
            .rect = {
                .left = 0,
                .top = 0,
                .width = 96,
                .height = 32,
            }
        };

        const Sprite spriteOnClick{
            .texture = LARGE_BUTTON_CLICK_TEXTURE,
            .scale = {props.scale, props.scale},
            .rect = {
                .left = 0,
                .top = 0,
                .width = 96,
                .height = 32,
            }
        };

        const float absScale = std::abs(props.scale);

        ref.set(
            props.pos,
            Size{static_cast<float>(sprite.rect.width) * absScale, static_cast<float>(sprite.rect.height) * absScale},
            sprite,
            AnimatedSprite(sprite, spriteOnHover, spriteOnClick, props.animated)
        );
    }
};

struct SpriteWithText {
    struct Props {
        Sprite image;
        std::string text;
        ResourceIndex font;
        uint32_t fontSize;
    };

    static Props Default() {
        return Props{
            .image = {
                .texture = 0,
                .scale = {0, 0},
                .rect = {
                    .left = 0,
                    .top = 0,
                    .width = 0,
                    .height = 0,
                }
            },
            .text = "",
            .font = 0,
            .fontSize = 16
        };
    }

    static void construct(ecs::EntityRef& ref, const Props& props) {
        const Sprite sprite{
            .texture = props.image.texture,
            .scale = {
                props.image.scale.x,
                props.image.scale.y
            },
            .rect = {
                .left = props.image.rect.left,
                .top = props.image.rect.top,
                .width = props.image.rect.width,
                .height = props.image.rect.height,
            }
        };

        ref.set(
            Size{static_cast<float>(props.image.rect.width), static_cast<float>(props.image.rect.height)},
            sprite,
            TextOnSpriteComponent(props.font, props.text, Color{0, 0, 0, 0}, props.fontSize)
        );
    }
};