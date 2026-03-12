#pragma once
#include <cstdint>

#include "ecs/World.hpp"
#include "reflection/rayflect.hpp"

struct Size {
    float width;
    float height;
};

struct Vec2Reflect {
    static StructDef *def() {
        static StructDef def{};
        static bool initialized = false;
        if (!initialized) {
            def.member<float>("x")->member<float>("y");
            initialized = true;
        }
        return &def;
    }
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

struct GlobalPosition : Vec2Reflect {
};

struct Position : Required<GlobalPosition>, Vec2Reflect {
    float x, y;

    Position(const float x, const float y) : x(x), y(y) {
    }
};
