#pragma once

#include <string>
#include "Types.hpp"

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

    virtual bool isKeyPressed(KeyboardCode code) = 0;

    virtual ResourceIndex loadTexture(const std::string &path) = 0;

    virtual void drawSprite(GlobalPosition pos, const Sprite &spr) = 0;

    virtual ResourceIndex loadFont(const std::string &path, unsigned int size) = 0;

    virtual void drawText(GlobalPosition pos, ResourceIndex handle, const char *str, Color color) = 0;

    [[nodiscard]] virtual IVec2 getMousePosition() const = 0;

    [[nodiscard]] virtual bool isMouseButtonPressed(int button) const = 0;

    [[nodiscard]] virtual bool wasMouseButtonReleased(int button) const = 0;

    virtual void drawRect(GlobalPosition pos, Size size, Color color) = 0;

    virtual void drawRectOutline(GlobalPosition pos, Size size, Color fillColor, Color outlineColor,
                                 float outlineThickness) = 0;
};
