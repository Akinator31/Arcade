#include <iostream>

#include "engine/Graphics.hpp"
#include "engine/ecs/World.hpp"
#include "engine/plugins/CliPlugin.hpp"
#include "engine/reflection/rayflect.hpp"


struct NoIntegrate;


enum class GameState {
    Menu,
    Game
};


struct Player;
struct Enemy;

[[noreturn]] int main() {
    ecs::World world;

    world.create().add<Position, Enemy>().set(Name{"enemy"});
    world.create().add<Player>().set(Name{"player"}, Position{10., 1.});
    world.create().add<Position, Player>().set(Name{"sasa"});

    world.plugin<CliPlugin>(CliMode::Server, 4040);

    while (true) {
        world.progress();
    }
}
