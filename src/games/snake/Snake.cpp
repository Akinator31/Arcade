#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugin.hpp"
#include "engine/plugins/score/ScorePlugin.hpp"
#include "games/common/GameMenuReturnPlugin.hpp"
#include "IGameModule.hpp"
#include "SnakeGameplayPlugin.hpp"

struct SnakeScene : DefaultPlugin {
};

namespace {
    constexpr ResourceIndex kHudFont = 0;
}

extern "C" IGameModule *load() {
    auto *engine = new Engine("Snake", [](Engine &engine, [[maybe_unused]] IDisplayModule *api) {
        ecs::World &world = engine.scene<SnakeScene>();

        engine.setScene<SnakeScene>();
        world.plugin<GameMenuReturnPlugin>();
        world.plugin<ScorePlugin>(ScorePlugin::Config{
            .font = kHudFont,
            .lives = 1,
            .x = 24.f,
            .y = 18.f,
            .fontSize = 28
        });
        world.plugin<SnakeGameplayPlugin>(SnakeGameplayPlugin::Config{
            .width = 21,
            .height = 15,
            .tileSize = 44.f,
            .originX = 178.f,
            .originY = 120.f,
            .moveInterval = 0.15f,
            .wrap = false
        });

        score_plugin_api::setMessage(world, "ZQSD OR ARROWS", Color{180, 220, 180, 255});
    }, {
        Resource::font("./assets/Fonts/pixellari.ttf")
    },
    []([[maybe_unused]] Engine &engine, IDisplayModule *api) {
        api->setClearColor({12, 18, 12, 255});
        api->setWindowSize({1280, 900});
    });

    return reinterpret_cast<IGameModule *>(engine);
}

extern "C" void unload(const IGameModule *game) {
    delete game;
}

extern "C" {
LibType LIB_TYPE = GAME;
}
