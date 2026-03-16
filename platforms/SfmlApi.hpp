#pragma once
#include "../src/engine/Graphics.hpp"
#include <SFML/Graphics.hpp>

struct FontRecord {
    int size;
    sf::Font font;
};

class SfmlGraphicsApi : public GraphicsApi {
    sf::RenderWindow window;
    sf::RectangleShape rectShape;
    sf::CircleShape circleShape;
    sf::Sprite sprite;
    std::vector<sf::Texture> images;
    std::vector<FontRecord> fonts;
    mutable sf::Text textShape;
    sf::Clock clock;
    float deltaTime = 0.f;
    bool mouseReleasedThisFrame = false;

public:
    void init() override;

    void shutdown() override;

    bool isWindowOpen() override;

    void beginFrame() override;

    void endFrame() override;

    void setWindowSize(USize rect) override;

    USize getWindowSize() const override;

    float getDeltaTime() const override;

    ImageHandle loadImage(const std::string &path) override;

    FontHandle loadFont(const std::string &path, int size) override;

    bool isKeyPressed(KeyboardCode code) override;

    void setFrameLimit(uint) override;

    IVec2 getMousePosition() const override;

    bool isMouseButtonPressed(int button) const override;

    bool wasMouseButtonReleased(int button) const override;

    void drawRect(GlobalPosition pos, Size size, Color color) override;

    void drawRectOutline(GlobalPosition pos, Size size, Color fillColor, Color outlineColor,
                         float outlineThickness) override;


    void drawSprite(GlobalPosition pos, const Sprite &spr) override;

    void drawText(GlobalPosition pos, FontHandle handle, const char *str, Color color) override;
};
