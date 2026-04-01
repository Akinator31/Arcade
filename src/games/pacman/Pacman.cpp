#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"
#include "engine/plugins/tilemap/TileMapPlugin.hpp"
#include "IGameModule.hpp"

struct DefaultScene : MenuScenePlugin {};

namespace {
    constexpr float kTileSize = 64.f;

    const char *map =
            "############################\n"
            "#            ##            #\n"
            "# #### ##### ## ##### #### #\n"
            "# #### ##### ## ##### #### #\n"
            "#                          #\n"
            "###### ## ######## ## ######\n"
            "#      ##    ##    ##      #\n"
            "# ########## ## ########## #\n"
            "#                          #\n"
            "############################\n";
}

extern "C" IGameModule *load() {
    auto *engine = new Engine("Pacman", [](Engine &engine, IDisplayModule *api) {
        ecs::World &world = engine.scene<DefaultScene>();
        engine.setScene<DefaultScene>();
        world.plugin<TileMapPlugin>();
        api->setClearColor({0, 0, 0, 255});

        const ecs::EntityRef wall = world.create().add<IsGround>().set(
            RigidBody::RIGID,
            Position{0.f, 0.f},
            Size{kTileSize, kTileSize},
            Color::blue()
        );
        TileMapPlugin::spawn(wall, {.map = map, .wall = '#'});

        world.create().set(
            Position{1.1f * kTileSize, 1.1f * kTileSize},
            Size{kTileSize / 2.f, kTileSize / 2.f},
            CharacterController{
                .left = Q,
                .right = D,
                .up = Z,
                .down = S,
                .speed = 220.f
            },
            Color::white(),
            RigidBody::RIGID
        );
        world.create().set(
            Position{4.f * kTileSize, 2.f * kTileSize},
            Size{kTileSize * 0.8f, kTileSize * 0.8f},
            Color::blue()
        );
    }, {});
    return reinterpret_cast<IGameModule *>(engine);
}

extern "C" void unload(const IGameModule *game) {
    delete game;
}
