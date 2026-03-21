#pragma once
#include "UiPlugin.hpp"
#include "arcade/Types.hpp"
#include "ecs/World.hpp"

struct Button {
    struct Props {
        Sprite image;
        Sprite imageOnHover;
        Position pos;
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
            .imageOnHover = {
                .texture = 0,
                .scale = {0, 0},
                .rect = {
                    .left = 0,
                    .top = 0,
                    .width = 0,
                    .height = 0,
                }
            },
            .pos = {0, 0}
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

        const Sprite spriteOnHover{
            .texture = props.imageOnHover.texture,
            .scale = {
                props.imageOnHover.scale.x,
                props.imageOnHover.scale.y
            },
            .rect = {
                .left = props.imageOnHover.rect.left,
                .top = props.imageOnHover.rect.top,
                .width = props.imageOnHover.rect.width,
                .height = props.imageOnHover.rect.height,
            }
        };

        ref.set(
            props.pos,
            Size{static_cast<float>(props.image.rect.width), static_cast<float>(props.image.rect.height)},
            sprite,
            HoveredSensorComponent{},
            ImageOnHoverComponent{
                .image = sprite,
                .imageOnHover = spriteOnHover
            }
        );
    }
};