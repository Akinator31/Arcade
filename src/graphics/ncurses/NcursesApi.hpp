#pragma once

#include "IDisplayModule.hpp"

#include <ncurses.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class NcursesGraphicsApi : public IDisplayModule {
    bool windowOpen = false;
    USize termSize{};
    USize referenceSize{};
    Color clearColor{0, 0, 0, 255};
    bool mouseReleasedThisFrame = false;
    bool mousePressed = false;
    IVec2 mousePos{0, 0};
    std::unordered_set<int> pressedKeys;
    std::unordered_map<int, short> colorPairCache;
    short nextPairId = 1;

    static short toNcursesColor(const Color& c);
    short getColorPair(const Color& fg, const Color& bg);
    static KeyboardCode ncursesToKeyboardCode(int ch);

    float scaleX(float x) const;
    float scaleY(float y) const;
    float invScaleX(float x) const;
    float invScaleY(float y) const;

public:
    NcursesGraphicsApi() {}

    void init() override;
    void shutdown() override;
    bool isWindowOpen() override;
    void beginFrame() override;
    void endFrame() override;
    void setWindowSize(USize size) override;
    USize getWindowSize() const override;
    bool isKeyPressed(KeyboardCode code) override;
    IVec2 getMousePosition() const override;
    bool isMouseButtonPressed(int button) const override;
    bool wasMouseButtonReleased(int button) const override;
    void drawRect(GlobalPosition pos, Size size, Color color) override;
    void drawRectOutline(GlobalPosition pos, Size size,
                         Color fillColor, Color outlineColor,
                         float thickness) override;
    void drawSprite(GlobalPosition pos, const Sprite& spr) override;
    void drawText(GlobalPosition pos, ResourceIndex,
                  const char* str, Color, uint32_t) override;
    void setClearColor(Color color) override;
    void loadResources(const std::vector<Resource>&) override;
};
