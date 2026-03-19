#pragma once

#include <cstdint>

#include "Rayflect.hpp"

struct Size {
    float width;
    float height;

    rayflect(value, {
             value->member<float>("width");
             value->member<float>("height");
             })
};

struct USize {
    uint32_t width;
    uint32_t height;

    rayflect(value, {
             value->member<uint32_t>("width");
             value->member<uint32_t>("height");
             })
};

using Resolution = USize;
using ResourceIndex = uint16_t;

struct Vec2 {
    float x;
    float y;
};

struct IVec2 {
    int x;
    int y;
};

struct Vec2Reflect {
    rayflect(value, {
             value->member<float>("x");
             value->member<float>("y");
             })
};

struct IVec2Reflect {
    rayflect(value, {
             value->member<int>("x");
             value->member<int>("y");
             })
};

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    static Color RED() {
        return Color{255, 0, 0, 255};
    }

    static Color BLUE() {
        return Color{0, 0, 255, 255};
    }

    static Color WHITE() {
        return Color{255, 255, 255, 255};
    }

    rayflect(value, {
             value->member<uint8_t>("r");
             value->member<uint8_t>("g");
             value->member<uint8_t>("b");
             value->member<uint8_t>("a");
             })
};

struct Node {
    Color backgroundColor;
    Color borderColor;
    float borderWidth;
};

struct GlobalPosition : Vec2Reflect {
    float x = 0.f;
    float y = 0.f;

    GlobalPosition() = default;

    GlobalPosition(const float x_value, const float y_value) : x(x_value), y(y_value) {
    }
};

struct Velocity : Vec2Reflect {
    float x = 0.f;
    float y = 0.f;

    Velocity() = default;

    Velocity(const float x_value, const float y_value) : x(x_value), y(y_value) {
    }
};

struct FontHandle {
    uint32_t handle;
};

struct SpriteRect {
    unsigned int left;
    unsigned int top;
    unsigned int width;
    unsigned int height;
};

struct Sprite {
    ResourceIndex texture;
    Vec2 scale;
    SpriteRect rect;

    rayflect(value, {
             value->member<uint16_t>("texture");
             value->member<float>("scale_x");
             value->member<float>("scale_y");
             value->member<unsigned int>("rect_left");
             value->member<unsigned int>("rect_top");
             value->member<unsigned int>("rect_width");
             value->member<unsigned int>("rect_height");
             })
};

enum KeyboardCode {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    ArrowUp,
    ArrowDown,
    ArrowRight,
    ArrowLeft,
    Space,
    CtrlLeft,
    Shift,
    F11,
    Escape,
    None
};

enum class ResourceType {
    Texture,
    Font,
    Sound,
};

struct Resource {
    ResourceType type;
    std::string path;

    static Resource texture(const std::string &path) {
        return {
            .type = ResourceType::Texture,
            .path = path
        };
    };

    static Resource font(const std::string &path) {
        return {
            .type = ResourceType::Font,
            .path = path
        };
    };
};
