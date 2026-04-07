#include "SfmlApi.hpp"

#include <SFML/Graphics.hpp>
#include <bit>
#include <cmath>
#include <iostream>

void SfmlGraphicsApi::init() {
    this->window.create(sf::VideoMode::getDesktopMode(), "game", sf::Style::Default);
    this->window.setVerticalSyncEnabled(false);
    this->window.setFramerateLimit(240);
}

void SfmlGraphicsApi::shutdown() {
    this->window.close();
}

bool SfmlGraphicsApi::isWindowOpen() {
    return this->window.isOpen();
}

void SfmlGraphicsApi::beginFrame() {
    this->mouseReleasedThisFrame = false;

    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        
        if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                window.close();
        }

        if (event->is<sf::Event::MouseButtonReleased>()) {
            this->mouseReleasedThisFrame = true;
        }
        if (const auto *value = event->getIf<sf::Event::Resized>()) {
            sf::View v(sf::FloatRect{{0, 0}, {static_cast<float>(value->size.x), static_cast<float>(value->size.y)}});
            window.setView(v);
        }
    }


    this->window.clear({this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a});
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

bool SfmlGraphicsApi::isKeyPressed(const KeyboardCode code) {
    sf::Keyboard::Key sfmlCode;

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
        case ArrowLeft:
            sfmlCode = sf::Keyboard::Key::Left;
            break;
        case Enter:
            sfmlCode = sf::Keyboard::Key::Enter;
            break;
        case ArrowRight:
            sfmlCode = sf::Keyboard::Key::Right;
            break;
        case ArrowUp:
            sfmlCode = sf::Keyboard::Key::Up;
            break;
        case ArrowDown:
            sfmlCode = sf::Keyboard::Key::Down;
            break;
        case Space:
            sfmlCode = sf::Keyboard::Key::Space;
            break;
        case F11:
            sfmlCode = sf::Keyboard::Key::F11;
            break;
        default:
            sfmlCode = sf::Keyboard::Key::Unknown;
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
        return sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    if (button == 1)
        return sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
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

void SfmlGraphicsApi::loadResources(const std::vector<Resource> &resources) {
    this->_resources.clear();
    for (const auto &[path, type]: resources) {
        if (type == ResourceType::Texture) {
            this->_resources.emplace_back(ResourceRecord::texture(sf::Texture(path)));
        }
        if (type == ResourceType::Font) {
            std::cout << "load font" << std::endl;
            this->_resources.emplace_back(ResourceRecord::font(sf::Font(path)));
        }
    }
}

void SfmlGraphicsApi::drawText(GlobalPosition pos, const ResourceIndex handle, const char *str, const Color color,
                               const uint32_t fontSize) {
    this->textShape.setFont(this->_resources[handle].f);
    this->textShape.setString(str);
    this->textShape.setCharacterSize(fontSize);
    this->textShape.setFillColor(std::bit_cast<sf::Color>(color));
    this->textShape.setPosition({pos.x, pos.y});
    this->window.draw(this->textShape);
}

void SfmlGraphicsApi::drawSprite(const GlobalPosition pos, const Sprite &spr) {
    this->sprite.setTexture(this->_resources[spr.texture].t);
    this->sprite.setTextureRect(std::bit_cast<sf::IntRect>(spr.rect));

    const float originX = static_cast<float>(spr.rect.width) / 2.f;
    const float originY = static_cast<float>(spr.rect.height) / 2.f;
    this->sprite.setOrigin({originX, originY});

    const float offsetX = originX * std::abs(spr.scale.x);
    const float offsetY = originY * std::abs(spr.scale.y);
    this->sprite.setPosition({pos.x + offsetX, pos.y + offsetY});

    this->sprite.setScale({spr.scale.x, spr.scale.y});
    this->window.draw(this->sprite);
}

void SfmlGraphicsApi::setClearColor(const Color color) {
    this->clearColor = color;
}

extern "C" {
LibType LIB_TYPE = GRAPHIC;

IDisplayModule *load() {
    return new SfmlGraphicsApi();
}

void unload(const IDisplayModule *api) {
    delete api;
}
}
