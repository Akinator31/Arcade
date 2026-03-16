#include "SfmlApi.hpp"
#include <SFML/Graphics.hpp>
#include <bit>
#include <cmath>
#include <stdexcept>

void SfmlGraphicsApi::init() {
    this->window.create({1920, 1080}, "game", sf::Style::Default);
    this->window.setVerticalSyncEnabled(false);
    this->window.setFramerateLimit(120);
}

void SfmlGraphicsApi::shutdown() {
    this->window.close();
}

bool SfmlGraphicsApi::isWindowOpen() {
    return this->window.isOpen();
}

void SfmlGraphicsApi::beginFrame() {
    sf::Event event;
    this->mouseReleasedThisFrame = false;

    while (this->window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            this->window.close();
        }
        if (event.type == sf::Event::MouseButtonReleased) {
            this->mouseReleasedThisFrame = true;
        }
        if (event.type == sf::Event::Resized) {
            sf::View v(sf::FloatRect(0.f, 0.f, static_cast<float>(event.size.width),
                                     static_cast<float>(event.size.height)));
            window.setView(v);
        }
    }

    this->window.clear();
    this->deltaTime = this->clock.restart().asSeconds();
}

void SfmlGraphicsApi::endFrame() {
    this->window.display();
}

void SfmlGraphicsApi::setWindowSize(USize rect) {
    this->window.setSize({rect.width, rect.height});
}

USize SfmlGraphicsApi::getWindowSize() const {
    const auto size = this->window.getSize();
    return {size.x, size.y};
}

float SfmlGraphicsApi::getDeltaTime() const {
    return this->deltaTime;
}

bool SfmlGraphicsApi::isKeyPressed(const KeyboardCode code) {
    sf::Keyboard::Key sfmlCode = sf::Keyboard::Key::KeyCount;

#define key(name)                                                                                  \
    case KeyboardCode::name:                                                                       \
        sfmlCode = sf::Keyboard::Key::name;                                                        \
        break

    switch (code) {
        key(A);
        key(B);
        key(C);
        key(D);
        key(E);
        key(F);
        key(G);
        key(H);
        key(I);
        key(J);
        key(K);
        key(L);
        key(M);
        key(N);
        key(O);
        key(P);
        key(Q);
        key(R);
        key(S);
        key(T);
        key(U);
        key(V);
        key(W);
        key(X);
        key(Y);
        key(Z);
        case KeyboardCode::ArrowLeft:
            sfmlCode = sf::Keyboard::Key::Left;
            break;
        case KeyboardCode::ArrowRight:
            sfmlCode = sf::Keyboard::Key::Right;
            break;
        case KeyboardCode::ArrowUp:
            sfmlCode = sf::Keyboard::Key::Up;
            break;
        case KeyboardCode::ArrowDown:
            sfmlCode = sf::Keyboard::Key::Down;
            break;
        case KeyboardCode::Space:
            sfmlCode = sf::Keyboard::Key::Space;
            break;
        case KeyboardCode::F11:
            sfmlCode = sf::Keyboard::Key::F11;
            break;
        default:
            sfmlCode = sf::Keyboard::Key::KeyCount;
    }
#undef key

    return sf::Keyboard::isKeyPressed(sfmlCode);
}

IVec2 SfmlGraphicsApi::getMousePosition() const {
    const auto pixelPos = sf::Mouse::getPosition(this->window);
    const auto worldPos = this->window.mapPixelToCoords(pixelPos);
    return {static_cast<int>(worldPos.x), static_cast<int>(worldPos.y)};
}

bool SfmlGraphicsApi::isMouseButtonPressed(const int button) const {
    if (button == 0)
        return sf::Mouse::isButtonPressed(sf::Mouse::Left);
    if (button == 1)
        return sf::Mouse::isButtonPressed(sf::Mouse::Right);
    return false;
}

bool SfmlGraphicsApi::wasMouseButtonReleased(const int button) const {
    if (button == 0)
        return this->mouseReleasedThisFrame;
    return false;
}

void SfmlGraphicsApi::drawRect(GlobalPosition pos, Size size, const Color color) {
    this->rectShape.setPosition({pos.x, pos.y});
    this->rectShape.setSize({size.width, size.height});
    this->rectShape.setFillColor(std::bit_cast<sf::Color>(color));
    this->rectShape.setOutlineThickness(0.f);
    this->window.draw(this->rectShape);
}

void SfmlGraphicsApi::drawRectOutline(GlobalPosition pos, Size size, const Color fillColor,
                                      const Color outlineColor, const float outlineThickness) {
    this->rectShape.setPosition({pos.x, pos.y});
    this->rectShape.setSize({size.width, size.height});
    this->rectShape.setFillColor(std::bit_cast<sf::Color>(fillColor));
    this->rectShape.setOutlineThickness(outlineThickness);
    this->rectShape.setOutlineColor(std::bit_cast<sf::Color>(outlineColor));
    this->window.draw(this->rectShape);
}

FontHandle SfmlGraphicsApi::loadFont(const std::string &path, const int size) {
    this->fonts.emplace_back();
    this->fonts.back().size = size;
    if (!this->fonts.back().font.loadFromFile(path)) {
        this->fonts.pop_back();
        throw std::runtime_error("Failed to load font: " + path);
    }
    return {.handle = static_cast<uint32_t>(this->fonts.size()) - 1};
}

void SfmlGraphicsApi::drawText(GlobalPosition pos, const FontHandle handle, const char *str, const Color color) {
    this->textShape.setFont(this->fonts[handle.handle].font);
    this->textShape.setString(str);
    this->textShape.setCharacterSize(static_cast<unsigned int>(this->fonts[handle.handle].size));
    this->textShape.setFillColor(std::bit_cast<sf::Color>(color));
    this->textShape.setPosition({pos.x, pos.y});
    this->window.draw(this->textShape);
}


ImageHandle SfmlGraphicsApi::loadImage(const std::string &path) {
    sf::Texture img;
    img.loadFromFile(path);

    this->images.push_back(std::move(img));

    return {.handle = static_cast<uint32_t>(this->images.size()) - 1};
}

void SfmlGraphicsApi::setFrameLimit(const uint value) {
    this->window.setFramerateLimit(value);
}

void SfmlGraphicsApi::drawSprite(const GlobalPosition pos, const Sprite &spr) {
    this->sprite.setTexture(this->images[spr.handle.handle]);
    this->sprite.setTextureRect(std::bit_cast<sf::IntRect>(spr.rect));

    const float originX = static_cast<float>(spr.rect.width) / 2.f;
    const float originY = static_cast<float>(spr.rect.height) / 2.f;
    this->sprite.setOrigin(originX, originY);

    const float offsetX = originX * std::abs(spr.scale.x);
    const float offsetY = originY * std::abs(spr.scale.y);
    this->sprite.setPosition(pos.x + offsetX, pos.y + offsetY);

    this->sprite.setScale(spr.scale.x, spr.scale.y);
    this->window.draw(this->sprite);
}

extern "C" {
GraphicsApi *create() {
    return new SfmlGraphicsApi();
}

void destroy(const GraphicsApi *api) {
    delete api;
}
}
