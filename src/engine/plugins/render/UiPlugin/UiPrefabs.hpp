#pragma once
#include "UiPlugin.hpp"
#include "arcade/Types.hpp"
#include "ecs/World.hpp"

struct Button {
    struct Props {
        ResourceIndex image;
        ResourceIndex hoverImage;
        float width;
        float height;
        float posX;
        float posY;
    };

    static Props Default() {
        return Props {.image = 0, .hoverImage = 0, .width = 300, .height = 300, .posX = 100, .posY = 100};
    }

    static void construct(ecs::EntityRef& ref, const Props& props) {
        ref.set(
            Position{props.posX, props.posY},
            Size{props.width, props.height},
            Sprite{
                .texture = props.image, .scale = {1, 1},
                .rect = {
                    .left = 0, .top = 0, .width = static_cast<uint>(props.width),
                    .height = static_cast<uint>(props.height)
                }
            },
            HoveredSensorComponent{},
            HoveredButtonComponent{.image = props.image, .hoverImage = props.hoverImage}
        );
    }
};