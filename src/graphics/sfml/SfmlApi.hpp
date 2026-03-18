#pragma once

#include "arcade/GraphicsApi.hpp"

#include <SFML/Graphics.hpp>

struct FontRecord {
    int size{};
    sf::Font font;
};

class SfmlGraphicsApi : public GraphicsApi {
    sf::RenderWindow window;
    sf::RectangleShape rectShape;
    sf::Sprite sprite;
    sf::Texture defaultTexture;
    sf::Font defaultFont;
    std::vector<sf::Texture> images;
    std::vector<FontRecord> fonts;
    mutable sf::Text textShape;
    sf::Clock clock;
    float deltaTime = 0.f;
    bool mouseReleasedThisFrame = false;

public:
    SfmlGraphicsApi() : rectShape({0, 0}), sprite(defaultTexture), textShape(defaultFont) {
    }

    void init() override;

    void shutdown() override;

    bool isWindowOpen() override;

    void beginFrame() override;

    void endFrame() override;

    void setWindowSize(USize rect) override;

    USize getWindowSize() const override;

    bool isKeyPressed(KeyboardCode code) override;

    ResourceIndex loadTexture(const std::string &path) override;

    ResourceIndex loadFont(const std::string &path, unsigned int size) override;

    IVec2 getMousePosition() const override;

    bool isMouseButtonPressed(int button) const override;

    bool wasMouseButtonReleased(int button) const override;

    void drawRect(GlobalPosition pos, Size size, Color color) override;

    void drawRectOutline(GlobalPosition pos, Size size, Color fillColor, Color outlineColor,
                         float outlineThickness) override;


    void drawSprite(GlobalPosition pos, const Sprite &spr) override;

    void drawText(GlobalPosition pos, ResourceIndex handle, const char *str, Color color) override;
};
