#pragma once
#include <cstdint>

#include "ecs/World.hpp"

struct Size {
    float width;
    float height;
};

struct Vec2 {
    float x;
    float y;
};

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct Node {
    Color backgroundColor;
    Color borderColor;
    float borderWidth;
};

struct GlobalPosition : Vec2 {
};

struct Position : Vec2, Required<GlobalPosition> {
    Position() = default;

    Position(const float x, const float y) : Vec2{x, y} {
    }
};
