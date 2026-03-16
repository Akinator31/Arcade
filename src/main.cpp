#include <bits/this_thread_sleep.h>
#include "engine/Graphics.hpp"
#include "engine/dynamic/DynamicLoader.hpp"
#include "engine/dynamic/Loader.hpp"
#include "engine/ecs/World.hpp"
#include "engine/plugins/cli/CliPlugin.hpp"
#include "engine/plugins/core_components/CoreComponentsPlugin.hpp"
#include "engine/plugins/render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"
#include "engine/plugins/render/RenderPlugin/RenderPlugin.hpp"

struct NoIntegrate;


enum class GameState {
    Menu,
    Game
};


struct Player;
struct Enemy;


struct SpriteAnimation {
    Timer timer;
    uint8_t start{};
    uint8_t end{};
    uint8_t index{};
};

struct SpriteAtlas {
    uint16_t tile_width;
    uint16_t tile_height;
    uint16_t rows;
    uint16_t cols;
};

int main() {
    ecs::World world;

    GraphicsApiLoader graphicsLoader("./libsfml_api.so");

    GraphicsApi *api = graphicsLoader.call<Create>();
    api->init();

    world.plugin<CoreComponentsPlugin>();
    world.plugin<CliPlugin>(CliMode::Server, 4040);
    world.plugin<PositionPropagationPlugin>();
    world.plugin<RenderPlugin>();

    world.create().set(
        Size{100, 100},
        Position{100, 100},
        Color{255, 0, 0, 255},
        Name{"Parent"}
    ).child().set(Size{100, 100},
                  Position{100, 100},
                  Color{255, 0, 0, 255}, Name{"Child"});

    world.singleton_init<GraphicsApi>(api);

    while (api->isWindowOpen()) {
        api->beginFrame();
        world.progress();
        api->endFrame();
    }
    return 0;
}
