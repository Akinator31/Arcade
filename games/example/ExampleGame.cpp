#include "ExampleGame.hpp"

#include "engine/Graphics.hpp"
#include "engine/plugins/DefaultPlugins.hpp"

ExampleGame::ExampleGame() : player(world.create()) {
    world.plugin<DefaultPlugin>();
    player.set(Position{100.f, 100.f});
}

std::string &ExampleGame::getName() {
    return this->name;
}

void ExampleGame::update(GraphicsApi *api) {
    auto *position = player.get<Position>();

    position->x += this->direction * 2.f;
    if (position->x < 0.f) {
        position->x = 0.f;
        this->direction = 1.f;
    }
    if (position->x > 700.f) {
        position->x = 700.f;
        this->direction = -1.f;
    }

    api->drawRect(std::bit_cast<GlobalPosition>(*position), Size{64.f, 64.f}, Color{255, 80, 80, 255});
}

extern "C" IGame *load() {
    return new ExampleGame();
}

extern "C" void unload(const IGame *game) {
    delete game;
}
