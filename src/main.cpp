#include <bits/this_thread_sleep.h>
#include "engine/Graphics.hpp"
#include "engine/ecs/World.hpp"
#include "engine/plugins/CliPlugin.hpp"


struct NoIntegrate;


enum class GameState {
    Menu,
    Game
};


struct Player;
struct Enemy;

int main() {
    ecs::World world;

    world.relation<Hierarchy>();

    world.create().add<Position, Enemy>().set(Name{"enemy"});
    const auto player = world.create().add<Player>().set(Name{"player"}, Position{10., 1.});
    world.create().add<Position, Player>().set(Name{"child"}).relate<Hierarchy>(player.id());

    world.plugin<CliPlugin>(CliMode::Server, 4040);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        world.progress();
    }
    return 0;
}
