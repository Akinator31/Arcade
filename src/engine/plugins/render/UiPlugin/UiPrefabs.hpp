#pragma once
#include "arcade/Types.hpp"
#include "ecs/World.hpp"

struct Button {
    struct Props {
        ResourceIndex image;
        float width;
        float height;
    };

    static void construct(ecs::EntityRef &ref, Props props = {
        .image = 0,
        .width = 0,
        .height = 0,
    }) {
        ref.set(
            Position {0, 0},
            Sprite {.texture = props.image, .scale = {1, 1}, .rect = {.left = 0, .top = 0, .width = static_cast<uint>(props.width), .height = static_cast<uint>(props.height)}},
            Size {props.width, props.height}
            );
    }
};