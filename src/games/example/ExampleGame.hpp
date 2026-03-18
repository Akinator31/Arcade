#pragma once

#include <string>

#include "arcade/GraphicsApi.hpp"
#include "arcade/IGame.hpp"
#include "engine/ecs/World.hpp"

class ExampleGame : public IGame {
    std::string name = "example";
    ecs::World world;
    ecs::EntityRef player;
    float direction = 1.f;

public:
    ExampleGame();

    std::string &getName() override;

    void update(GraphicsApi *api) override;
};
