#pragma once

#include "IDisplayModule.hpp"
#include <SFML/Graphics.hpp>

#include <ncurses.h>
#include <vector>
#include <variant>

class NcursesGraphicsApi : public IDisplayModule {
    bool windowOpen = false;
    USize windowSize{};

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

    void setClearColor(Color) override;

    void loadResources(const std::vector<Resource>&) override;

    static short toNcursesColor(const Color& c) {
        if (c.r > 200 && c.g < 100 && c.b < 100) return COLOR_RED;
        if (c.r < 100 && c.g > 200 && c.b < 100) return COLOR_GREEN;
        if (c.r < 100 && c.g < 100 && c.b > 200) return COLOR_BLUE;
        if (c.r > 200 && c.g > 200 && c.b < 100) return COLOR_YELLOW;
        if (c.r > 200 && c.g > 200 && c.b > 200) return COLOR_WHITE;
        if (c.r < 50 && c.g < 50 && c.b < 50) return COLOR_BLACK;

        return COLOR_WHITE;
    }

    short getColorPair(const Color& fg, const Color& bg) {
        static short nextPair = 1;

        short fgColor = toNcursesColor(fg);
        short bgColor = toNcursesColor(bg);

        init_pair(nextPair, fgColor, bgColor);
        return nextPair++;
    }
};
