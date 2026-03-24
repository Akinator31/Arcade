#pragma once

#include "IDisplayModule.hpp"

#include <SFML/Graphics.hpp>

using ResourceRecord = std::variant<sf::Font, sf::Texture>;

class SfmlGraphicsApi : public IDisplayModule {
    sf::RenderWindow window;
    sf::RectangleShape rectShape;
    sf::Sprite sprite;
    sf::Texture defaultTexture;
    sf::Font defaultFont;
    std::vector<ResourceRecord> _resources;
    mutable sf::Text textShape;
    sf::Clock clock;
    float deltaTime = 0.f;
    bool mouseReleasedThisFrame = false;
    Color clearColor;
    USize windowSize{};

public:
    SfmlGraphicsApi() : rectShape({0, 0}), sprite(defaultTexture), textShape(defaultFont),
                        clearColor(Color{0, 0, 0, 255}) {}

    void init() override;

    void shutdown() override;

    bool isWindowOpen() override;

    void beginFrame() override;

    void endFrame() override;

    void setWindowSize(USize rect) override;

    USize getWindowSize() const override;

    bool isKeyPressed(KeyboardCode code) override;

    void loadResources(const std::vector<Resource>& resources) override;

    IVec2 getMousePosition() const override;

    bool isMouseButtonPressed(int button) const override;

    bool wasMouseButtonReleased(int button) const override;

    void drawRect(GlobalPosition pos, Size size, Color color) override;

    void drawRectOutline(GlobalPosition pos, Size size, Color fillColor, Color outlineColor,
                         float outlineThickness) override;


    void drawSprite(GlobalPosition pos, const Sprite& spr) override;

    void drawText(GlobalPosition pos, ResourceIndex handle, const char* str, Color color, uint32_t fontSize) override;

    void setClearColor(Color color) override;
};