#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugin.hpp"
#include "engine/plugins/score/ScorePlugin.hpp"
#include "engine/plugins/tilemap/TileMapControlPlugin.hpp"
#include "engine/plugins/tilemap/TileMapPlugin.hpp"
#include "IGameModule.hpp"

struct DefaultScene : DefaultPlugin {
};

struct PlayerTag {
};

struct EnemyTag {
};

struct PelletTag {
};

struct PlayerSpawn {
    int x = 1;
    int y = 1;
    int direction = 1;
};

struct EnemySpawn {
    int x = 1;
    int y = 1;
    int direction = 0;
};

namespace {
    constexpr float kTileSize = 64.f;
    constexpr float kPlayerSize = 32.f;
    constexpr float kPelletSize = 10.f;
    constexpr ResourceIndex kHudFont = 0;

    constexpr auto map =
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

    float cellPosition(const int cell, const float size) {
        return static_cast<float>(cell) * kTileSize + (kTileSize - size) * 0.5f;
    }

    Position cellCenter(const int x, const int y, const float size) {
        return {cellPosition(x, size), cellPosition(y, size)};
    }

    bool overlapsEntity(const Position &aPos, const Size &aSize, const Position &bPos, const Size &bSize) {
        return aPos.x < bPos.x + bSize.width &&
               aPos.x + aSize.width > bPos.x &&
               aPos.y < bPos.y + bSize.height &&
               aPos.y + aSize.height > bPos.y;
    }

    void resetPlayer(ecs::World &world, const ecs::Entity entity) {
        const auto *spawn = world.get<PlayerSpawn>(entity);
        auto *control = world.get<TileMapAutoControl>(entity);
        const auto *size = world.get<Size>(entity);
        auto *position = world.get<Position>(entity);
        if (spawn == nullptr || control == nullptr || size == nullptr || position == nullptr) {
            return;
        }

        *position = cellCenter(spawn->x, spawn->y, size->width);
        control->direction = spawn->direction;
        control->wantedDirection = spawn->direction;
        control->fromX = spawn->x;
        control->fromY = spawn->y;
        control->toX = spawn->x;
        control->toY = spawn->y;
        control->moving = false;
        control->initialized = true;
        control->speed = 240.f;
        world.set(entity, Color{255, 255, 0, 255});
    }

    void resetEnemy(ecs::World &world, const ecs::Entity entity) {
        const auto *spawn = world.get<EnemySpawn>(entity);
        auto *control = world.get<TileMapRandomControl>(entity);
        const auto *size = world.get<Size>(entity);
        auto *position = world.get<Position>(entity);
        if (spawn == nullptr || control == nullptr || size == nullptr || position == nullptr) {
            return;
        }

        *position = cellCenter(spawn->x, spawn->y, size->width);
        control->direction = spawn->direction;
        control->fromX = spawn->x;
        control->fromY = spawn->y;
        control->toX = spawn->x;
        control->toY = spawn->y;
        control->moving = false;
        control->initialized = true;
    }
}

SYSTEM(PacmanPelletSys, With<PlayerTag, Position, Size>, On<Update>) {
    ITER(view) {
        const auto *playerPositions = view.column<Position>();
        const auto *playerSizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            const Position playerPosition = playerPositions[i];
            const Size playerSize = playerSizes[i];

            ecs::Entity pellet{};
            bool foundPellet = false;
            view.world.fetch<PelletTag, Position, Size>().iter([&](ArchetypeView &pelletView) {
                const auto *pelletPositions = pelletView.column<Position>();
                const auto *pelletSizes = pelletView.column<Size>();

                for (uint32_t pelletIndex = 0; pelletIndex < pelletView.count(); pelletIndex += 1) {
                    if (!overlapsEntity(playerPosition, playerSize, pelletPositions[pelletIndex],
                                        pelletSizes[pelletIndex])) {
                        continue;
                    }
                    pellet = pelletView.entity(pelletIndex);
                    foundPellet = true;
                    break;
                }
            });

            if (!foundPellet) {
                continue;
            }

            view.world.kill(pellet);
            score_plugin_api::addScore(view.world, 10);
            score_plugin_api::clearMessage(view.world);
        }
    }
};

SYSTEM(PacmanEnemyCollisionSys, With<PlayerTag, Position, Size>, On<Update>) {
    ITER(view) {
        const auto *hud = view.world.singleton_get<ScoreHudState>();
        if (hud == nullptr || hud->gameOver) {
            return;
        }

        const auto *playerPositions = view.column<Position>();
        const auto *playerSizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            const ecs::Entity player = view.entity(i);
            const Position playerPosition = playerPositions[i];
            const Size playerSize = playerSizes[i];

            bool hitEnemy = false;
            view.world.fetch<EnemyTag, Position, Size>().iter([&](ArchetypeView &enemyView) {
                const auto *enemyPositions = enemyView.column<Position>();
                const auto *enemySizes = enemyView.column<Size>();

                for (uint32_t enemyIndex = 0; enemyIndex < enemyView.count(); enemyIndex += 1) {
                    if (!overlapsEntity(playerPosition, playerSize, enemyPositions[enemyIndex],
                                        enemySizes[enemyIndex])) {
                        continue;
                    }
                    hitEnemy = true;
                    break;
                }
            });

            if (!hitEnemy) {
                continue;
            }

            score_plugin_api::loseLife(view.world);
            if (hud->gameOver) {
                view.world.set(player, Color{255, 0, 0, 255});
                if (auto *control = view.world.get<TileMapAutoControl>(player); control != nullptr) {
                    control->speed = 0.f;
                    control->moving = false;
                }
                continue;
            }

            resetPlayer(view.world, player);
            view.world.fetch<EnemyTag>().iter([&](const ArchetypeView &enemyView) {
                for (uint32_t enemyIndex = 0; enemyIndex < enemyView.count(); enemyIndex += 1) {
                    resetEnemy(view.world, enemyView.entity(enemyIndex));
                }
            });
        }
    }
};

extern "C" IGameModule *load() {
    auto *engine = new Engine("Pacman", [](Engine &engine, IDisplayModule *api) {
        ecs::World &world = engine.scene<DefaultScene>();

        engine.setScene<DefaultScene>();
        world.plugin<TileMapPlugin>();
        world.plugin<TileMapControlPlugin>();
        world.plugin<ScorePlugin>(ScorePlugin::Config{
            .font = kHudFont,
            .lives = 3,
            .x = 20.f,
            .y = 16.f,
            .fontSize = 30
        });
        world.registerComponent<PlayerTag>();
        world.registerComponent<EnemyTag>();
        world.registerComponent<PelletTag>();
        world.registerComponent<PlayerSpawn>();
        world.registerComponent<EnemySpawn>();
        world.system<PacmanPelletSys>();
        world.system<PacmanEnemyCollisionSys>();

        api->setClearColor({0, 0, 0, 255});
        api->setWindowSize({1800, 800});

        const ecs::EntityRef wall = world.create().add<IsGround>().set(
            Position{0.f, 0.f},
            Size{kTileSize, kTileSize},
            Color::blue(),
            RigidBody::RIGID
        );
        TileMapPlugin::spawn(wall, {.map = map, .tile = '#'});

        const ecs::EntityRef pellet = world.create().add<PelletTag>().set(
            Position{0.f, 0.f},
            Size{kTileSize, kTileSize},
            Color{255, 220, 120, 255}
        );
        TileMapPlugin::spawn(pellet, {.map = map, .tile = ' '});

        world.create().add<PlayerTag>().set(
            cellCenter(1, 1, kPlayerSize),
            Size{kPlayerSize, kPlayerSize},
            Color{255, 255, 0, 255},
            PlayerSpawn{1, 1, 1},
            TileMapAutoControl{
                .map = map,
                .wall = '#',
                .tileSize = kTileSize,
                .speed = 240.f,
                .left = Q,
                .right = D,
                .up = Z,
                .down = S,
                .direction = 1,
                .wantedDirection = 1
            }
        );

        world.create().add<EnemyTag>().set(
            cellCenter(6, 4, kPlayerSize),
            Size{kPlayerSize, kPlayerSize},
            Color{255, 0, 0, 255},
            EnemySpawn{6, 4, 0},
            TileMapRandomControl{
                .map = map,
                .wall = '#',
                .tileSize = kTileSize,
                .speed = 150.f,
                .direction = 0,
                .changeTimer = Timer{0.45f}
            }
        );

        world.create().add<EnemyTag>().set(
            cellCenter(20, 4, kPlayerSize),
            Size{kPlayerSize, kPlayerSize},
            Color{255, 105, 180, 255},
            EnemySpawn{20, 4, 2},
            TileMapRandomControl{
                .map = map,
                .wall = '#',
                .tileSize = kTileSize,
                .speed = 150.f,
                .direction = 2,
                .changeTimer = Timer{0.35f}
            }
        );
    }, {
        Resource::font("./assets/Fonts/pixellari.ttf")
    });

    return reinterpret_cast<IGameModule *>(engine);
}

extern "C" void unload(const IGameModule *game) {
    delete game;
}

extern "C" {
LibType LIB_TYPE = GAME;
}
