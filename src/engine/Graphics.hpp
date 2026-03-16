#pragma once
#include <cstdint>
#include "reflection/rayflect.hpp"


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
             }

    )
};

struct Vec2 {
    float x, y;
};

struct IVec2 {
    int x, y;
};

struct Vec2Reflect {
    rayflect(value, {
             value->member<float>("x");
             value->member<float>("y");
             }

    )
};

struct IVec2Reflect {
    rayflect(value, {
             value->member<int>("x");
             value->member<int>("y");
             }

    )
};

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

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
    float x;
    float y;

    GlobalPosition(const float x, const float y) : x(x), y(y) {
    }
};

struct Velocity : Vec2Reflect {
    float x;
    float y;

    Velocity() = default;

    Velocity(const float x, const float y) : x(x), y(y) {
    }
};

struct ImageHandle {
    uint32_t handle;
};

struct FontHandle {
    uint32_t handle;
};

struct SpriteRect {
    uint left;
    uint top;
    uint width;
    uint height;
};

struct Sprite {
    ImageHandle handle;

    Vec2 scale;

    SpriteRect rect;

    rayflect(value, {
             value->member<uint32_t>("font_handle");
             value->member<uint32_t>("font_size");

             value->member<float>("scale_x");
             value->member<float>("scale_y");

             value->member<float>("rect_left");
             value->member<float>("rect_top");
             value->member<float>("rect_width");
             value->member<float>("rect_height");
             }

    )
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
    None
};

class GraphicsApi {
public:
    virtual ~GraphicsApi() = default;

    virtual void init() = 0;

    virtual void shutdown() = 0;

    virtual bool isWindowOpen() = 0;

    virtual void beginFrame() = 0;

    virtual void endFrame() = 0;

    virtual void setWindowSize(USize rect) = 0;

    [[nodiscard]] virtual USize getWindowSize() const = 0;

    [[nodiscard]] virtual float getDeltaTime() const = 0;

    virtual void setFrameLimit(uint) = 0;

    virtual bool isKeyPressed(KeyboardCode code) = 0;

    virtual ImageHandle loadImage(const std::string &path) = 0;

    virtual void drawSprite(GlobalPosition pos, const Sprite &spr) = 0;

    virtual FontHandle loadFont(const std::string &path, int size) = 0;

    virtual void drawText(GlobalPosition pos, FontHandle handle, const char *str, Color color) = 0;

    [[nodiscard]] virtual IVec2 getMousePosition() const = 0;

    [[nodiscard]] virtual bool isMouseButtonPressed(int button) const = 0;

    [[nodiscard]] virtual bool wasMouseButtonReleased(int button) const = 0;

    virtual void drawRect(GlobalPosition pos, Size size, Color color) = 0;

    virtual void drawRectOutline(GlobalPosition pos, Size size, Color fillColor, Color outlineColor,
                                 float outlineThickness) = 0;
};
