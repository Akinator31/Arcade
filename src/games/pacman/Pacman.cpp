#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugin.hpp"
#include "engine/plugins/score/ScorePlugin.hpp"
#include "engine/plugins/tilemap/TileMapControlPlugin.hpp"
#include "engine/plugins/tilemap/TileMapPlugin.hpp"
#include "games/common/GameMenuReturnPlugin.hpp"
#include "IGameModule.hpp"

struct DefaultScene : DefaultPlugin {
};

struct PlayerTag {
};

struct GhostTag {
};

struct PelletTag {
};

struct PowerPelletTag {
};

struct LevelTileTag {
};

struct PlayerSpawn {
    int x = 0;
    int y = 0;
    int direction = 0;
};

struct GhostData {
    int homeX = 0;
    int homeY = 0;
    int exitX = 0;
    int exitY = 0;
    int direction = 0;
    float baseSpeed = 150.f;
    Color baseColor = Color::red();
    bool released = false;
    bool eyes = false;
    float healTimer = 0.f;
};

struct GhostControl : Required<Position, Size> {
    float speed = 150.f;
    int direction = 0;
    int fromX = 0;
    int fromY = 0;
    int toX = 0;
    int toY = 0;
    bool moving = false;
    bool initialized = false;
};

struct LevelState {
    int level = 1;
    int pelletsLeft = 0;
    int width = 0;
    int height = 0;
    int releaseX = 9;
    int releaseY = 11;
    float playerSpeed = 240.f;
    float ghostSpeed = 150.f;
    float releaseTimer = 10.f;
    float frightenedTimer = 0.f;
};

namespace {
    constexpr float kTileSize = 48.f;
    constexpr float kPlayerSize = 30.f;
    constexpr float kPelletSize = 8.f;
    constexpr float kPowerPelletSize = 18.f;
    constexpr float kEyesSpeed = 260.f;
    constexpr float kFrightenedSpeedFactor = 0.65f;
    constexpr float kFrightenedDuration = 10.f;
    constexpr float kGhostReleaseDelay = 10.f;
    constexpr float kGhostHealDelay = 2.f;
    constexpr ResourceIndex kHudFont = 0;

    constexpr auto kMap =
            "###################\n"
            "#o........#......o#\n"
            "#.###.###.#.###.###\n"
            "#.................#\n"
            "#.###.#.###.#.###.#\n"
            "#.....#.....#.....#\n"
            "###.#.#######.#.###\n"
            "#.....#12B34#.....#\n"
            " .....#BBBBB#..... \n"
            " .....#BBBBB#..... \n"
            "#.....#BBBBB#.....#\n"
            "###.#.#.....#.#.###\n"
            "#.....#..P..#.....#\n"
            "#.###.#.###.#.###.#\n"
            "#.....#.....#.....#\n"
            "#o###.......###..o#\n"
            "###################\n";

    const TileMapGrid &grid() {
        static const TileMapGrid value = TileMapPlugin::toGrid(kMap);
        return value;
    }

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

    bool isWall(const int x, const int y) {
        return TileMapPlugin::isTile(grid(), x, y, '#');
    }

    bool isInside(const int x, const int y) {
        return y >= 0 && y < static_cast<int>(grid().size()) &&
               x >= 0 && x < static_cast<int>(grid()[y].size());
    }

    int normalizeX(const int x) {
        const int width = static_cast<int>(grid()[0].size());
        if (x < 0) {
            return width - 1;
        }
        if (x >= width) {
            return 0;
        }
        return x;
    }

    float wrappedDistanceX(const int from, const int to) {
        const int width = static_cast<int>(grid()[0].size());
        const int raw = std::abs(to - from);
        return static_cast<float>(std::min(raw, width - raw));
    }

    float scoreDirection(const int fromX,
                         const int fromY,
                         const int direction,
                         const int targetX,
                         const int targetY,
                         const bool flee) {
        const int nextX = normalizeX(fromX + tilemap_control_impl::dx(direction));
        const int nextY = fromY + tilemap_control_impl::dy(direction);
        const float value = wrappedDistanceX(nextX, targetX) + static_cast<float>(std::abs(nextY - targetY));
        return flee ? value : -value;
    }

    void setControlCell(TileMapAutoControl &control, const int x, const int y) {
        control.fromX = x;
        control.fromY = y;
        control.toX = x;
        control.toY = y;
        control.moving = false;
        control.initialized = true;
    }

    void setControlCell(GhostControl &control, const int x, const int y) {
        control.fromX = x;
        control.fromY = y;
        control.toX = x;
        control.toY = y;
        control.moving = false;
        control.initialized = true;
    }

    void resetPlayer(ecs::World &world, const ecs::Entity entity) {
        const auto *spawn = world.get<PlayerSpawn>(entity);
        auto *control = world.get<TileMapAutoControl>(entity);
        auto *position = world.get<Position>(entity);
        const auto *size = world.get<Size>(entity);
        const auto *state = world.singleton_get<LevelState>();
        if (spawn == nullptr || control == nullptr || position == nullptr || size == nullptr || state == nullptr) {
            return;
        }

        *position = cellCenter(spawn->x, spawn->y, size->width);
        control->speed = state->playerSpeed;
        control->direction = spawn->direction;
        control->wantedDirection = spawn->direction;
        setControlCell(*control, spawn->x, spawn->y);
        world.set(entity, Color{255, 230, 0, 255});
    }

    void resetGhost(ecs::World &world, const ecs::Entity entity) {
        auto *ghost = world.get<GhostData>(entity);
        auto *control = world.get<GhostControl>(entity);
        auto *position = world.get<Position>(entity);
        const auto *size = world.get<Size>(entity);
        const auto *state = world.singleton_get<LevelState>();
        if (ghost == nullptr || control == nullptr || position == nullptr || size == nullptr || state == nullptr) {
            return;
        }

        ghost->released = false;
        ghost->eyes = false;
        ghost->healTimer = 0.f;
        control->speed = state->ghostSpeed;
        control->direction = ghost->direction;
        *position = cellCenter(ghost->homeX, ghost->homeY, size->width);
        setControlCell(*control, ghost->homeX, ghost->homeY);
        world.set(entity, ghost->baseColor);
    }

    void setFrightened(ecs::World &world) {
        auto *state = world.singleton_get<LevelState>();
        if (state == nullptr) {
            return;
        }

        state->frightenedTimer = kFrightenedDuration;
        score_plugin_api::setMessage(world, "POWER", Color{80, 160, 255, 255});
        world.fetch<GhostTag>().iter([&](const ArchetypeView &view) {
            for (uint32_t i = 0; i < view.count(); i += 1) {
                const ecs::Entity ghost = view.entity(i);
                auto *ghostData = world.get<GhostData>(ghost);
                auto *control = world.get<GhostControl>(ghost);
                if (ghostData == nullptr || control == nullptr || ghostData->eyes) {
                    continue;
                }
                control->speed = ghostData->baseSpeed * kFrightenedSpeedFactor;
                world.set(ghost, Color{70, 120, 255, 255});
            }
        });
    }

    void applyLevelSpeed(ecs::World &world) {
        const auto *state = world.singleton_get<LevelState>();
        if (state == nullptr) {
            return;
        }

        world.fetch<PlayerTag, TileMapAutoControl>().iter([&](ArchetypeView &view) {
            auto *controls = view.column<TileMapAutoControl>();
            for (uint32_t i = 0; i < view.count(); i += 1) {
                controls[i].speed = state->playerSpeed;
            }
        });

        world.fetch<GhostTag, GhostData, GhostControl>().iter([&](ArchetypeView &view) {
            auto *ghosts = view.column<GhostData>();
            auto *controls = view.column<GhostControl>();
            for (uint32_t i = 0; i < view.count(); i += 1) {
                controls[i].speed = state->frightenedTimer > 0.f
                        ? ghosts[i].baseSpeed * kFrightenedSpeedFactor
                        : ghosts[i].baseSpeed;
            }
        });
    }

    void clearLevelTiles(ecs::World &world) {
        std::vector<ecs::Entity> entities;
        world.fetch<LevelTileTag>().iter([&](const ArchetypeView &view) {
            for (uint32_t i = 0; i < view.count(); i += 1) {
                entities.push_back(view.entity(i));
            }
        });
        for (const ecs::Entity entity: entities) {
            world.kill(entity);
        }
    }

    void spawnLevelTiles(ecs::World &world) {
        auto *state = world.singleton_get<LevelState>();
        if (state == nullptr) {
            return;
        }

        clearLevelTiles(world);
        state->pelletsLeft = 0;
        state->height = static_cast<int>(grid().size());
        state->width = static_cast<int>(grid()[0].size());

        const ecs::EntityRef wall = world.create().add<LevelTileTag>().add<IsGround>().set(
            Position{0.f, 0.f},
            Size{kTileSize, kTileSize},
            Color::blue(),
            RigidBody::RIGID
        );
        TileMapPlugin::spawn(wall, {.map = kMap, .tile = '#'});

        for (int y = 0; y < state->height; y += 1) {
            for (int x = 0; x < state->width; x += 1) {
                const char tile = grid()[y][x];
                if (tile == '.' || tile == 'o') {
                    state->pelletsLeft += 1;
                }
                if (tile != '.') {
                    continue;
                }
                world.create().add<LevelTileTag>().add<PelletTag>().set(
                    cellCenter(x, y, kPelletSize),
                    Size{kPelletSize, kPelletSize},
                    Color{255, 210, 120, 255}
                );
            }
        }

        for (int y = 0; y < state->height; y += 1) {
            for (int x = 0; x < state->width; x += 1) {
                if (grid()[y][x] != 'o') {
                    continue;
                }
                world.create().add<LevelTileTag>().add<PowerPelletTag>().set(
                    cellCenter(x, y, kPowerPelletSize),
                    Size{kPowerPelletSize, kPowerPelletSize},
                    Color{255, 245, 170, 255}
                );
            }
        }
    }

    std::array<int, 2> findTile(const char needle) {
        for (int y = 0; y < static_cast<int>(grid().size()); y += 1) {
            for (int x = 0; x < static_cast<int>(grid()[y].size()); x += 1) {
                if (grid()[y][x] == needle) {
                    return {x, y};
                }
            }
        }
        return {0, 0};
    }
}

SYSTEM(PacmanWrapSys, With<Position, Size>, On<Update>) {
    ITER(view) {
        const auto *state = view.world.singleton_get<LevelState>();
        if (state == nullptr) {
            return;
        }

        auto *positions = view.column<Position>();
        const auto *sizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            auto &position = positions[i];
            const auto &size = sizes[i];

            if (view.world.has<TileMapAutoControl>(view.entity(i))) {
                auto *control = view.world.get<TileMapAutoControl>(view.entity(i));
                if (!control->moving && (control->fromX < 0 || control->fromX >= state->width)) {
                    control->fromX = normalizeX(control->fromX);
                    control->toX = control->fromX;
                    position.x = cellPosition(control->fromX, size.width);
                }
            }

            if (view.world.has<GhostControl>(view.entity(i))) {
                auto *control = view.world.get<GhostControl>(view.entity(i));
                if (!control->moving && (control->fromX < 0 || control->fromX >= state->width)) {
                    control->fromX = normalizeX(control->fromX);
                    control->toX = control->fromX;
                    position.x = cellPosition(control->fromX, size.width);
                }
            }
        }
    }
};

SYSTEM(PacmanPelletSys, With<PlayerTag, Position, Size>, On<Update>) {
    ITER(view) {
        auto *state = view.world.singleton_get<LevelState>();
        if (state == nullptr) {
            return;
        }

        const auto *playerPositions = view.column<Position>();
        const auto *playerSizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            const Position playerPosition = playerPositions[i];
            const Size playerSize = playerSizes[i];

            ecs::Entity hitPellet{};
            bool foundPellet = false;
            bool powerPellet = false;

            view.world.fetch<PelletTag, Position, Size>().iter([&](ArchetypeView &pelletView) {
                if (foundPellet) {
                    return;
                }
                const auto *positions = pelletView.column<Position>();
                const auto *sizes = pelletView.column<Size>();
                for (uint32_t pelletIndex = 0; pelletIndex < pelletView.count(); pelletIndex += 1) {
                    if (!overlapsEntity(playerPosition, playerSize, positions[pelletIndex], sizes[pelletIndex])) {
                        continue;
                    }
                    hitPellet = pelletView.entity(pelletIndex);
                    foundPellet = true;
                    break;
                }
            });

            if (!foundPellet) {
                view.world.fetch<PowerPelletTag, Position, Size>().iter([&](ArchetypeView &pelletView) {
                    const auto *positions = pelletView.column<Position>();
                    const auto *sizes = pelletView.column<Size>();
                    for (uint32_t pelletIndex = 0; pelletIndex < pelletView.count(); pelletIndex += 1) {
                        if (!overlapsEntity(playerPosition, playerSize, positions[pelletIndex], sizes[pelletIndex])) {
                            continue;
                        }
                        hitPellet = pelletView.entity(pelletIndex);
                        foundPellet = true;
                        powerPellet = true;
                        break;
                    }
                });
            }

            if (!foundPellet) {
                continue;
            }

            view.world.kill(hitPellet);
            state->pelletsLeft -= 1;
            score_plugin_api::addScore(view.world, powerPellet ? 50 : 10);
            if (powerPellet) {
                setFrightened(view.world);
            } else if (state->frightenedTimer <= 0.f) {
                score_plugin_api::clearMessage(view.world);
            }
        }
    }
};

SYSTEM(PacmanStateSys, On<Update>) {
    static void run(ecs::World &world) {
        auto *state = world.singleton_get<LevelState>();
        if (state == nullptr) {
            return;
        }

        if (state->releaseTimer > 0.f) {
            state->releaseTimer -= world.deltaTime;
        }

        if (state->frightenedTimer <= 0.f) {
            return;
        }

        state->frightenedTimer -= world.deltaTime;
        if (state->frightenedTimer > 0.f) {
            return;
        }

        state->frightenedTimer = 0.f;
        score_plugin_api::clearMessage(world);
        world.fetch<GhostTag, GhostData, GhostControl>().iter([&](ArchetypeView &view) {
            auto *ghosts = view.column<GhostData>();
            auto *controls = view.column<GhostControl>();
            for (uint32_t i = 0; i < view.count(); i += 1) {
                if (ghosts[i].eyes) {
                    continue;
                }
                controls[i].speed = ghosts[i].baseSpeed;
                world.set(view.entity(i), ghosts[i].baseColor);
            }
        });
    }
};

SYSTEM(PacmanGhostAiSys, With<GhostTag, GhostData, GhostControl, Position, Size>, On<Update>) {
    ITER(view) {
        const auto *state = view.world.singleton_get<LevelState>();
        if (state == nullptr) {
            return;
        }

        int playerX = 0;
        int playerY = 0;
        view.world.fetch<PlayerTag, TileMapAutoControl>().iter([&](ArchetypeView &playerView) {
            const auto *controls = playerView.column<TileMapAutoControl>();
            if (playerView.count() == 0) {
                return;
            }
            playerX = controls[0].fromX;
            playerY = controls[0].fromY;
        });

        auto *ghosts = view.column<GhostData>();
        auto *controls = view.column<GhostControl>();
        auto *positions = view.column<Position>();
        const auto *sizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            auto &ghost = ghosts[i];
            auto &control = controls[i];
            auto &position = positions[i];
            const auto &size = sizes[i];

            tilemap_control_impl::init(
                position, size, kTileSize,
                control.fromX, control.fromY, control.toX, control.toY, control.initialized);

            if (control.moving) {
                const Position target = cellCenter(control.toX, control.toY, size.width);
                if (tilemap_control_impl::moveToward(position, target, control.speed, view.world.deltaTime)) {
                    control.fromX = control.toX;
                    control.fromY = control.toY;
                    control.moving = false;
                    if (control.fromX < 0 || control.fromX >= state->width) {
                        control.fromX = normalizeX(control.fromX);
                        control.toX = control.fromX;
                        position.x = cellPosition(control.fromX, size.width);
                    }
                }
                continue;
            }

            if (ghost.eyes) {
                control.speed = kEyesSpeed;
                if (control.fromX == ghost.homeX && control.fromY == ghost.homeY && !control.moving) {
                    ghost.eyes = false;
                    ghost.released = false;
                    ghost.healTimer = kGhostHealDelay;
                    control.speed = 0.f;
                    view.world.set(view.entity(i), ghost.baseColor);
                    continue;
                }
            } else if (ghost.healTimer > 0.f) {
                ghost.healTimer -= view.world.deltaTime;
                control.speed = 0.f;
                if (ghost.healTimer <= 0.f) {
                    control.speed = state->ghostSpeed;
                }
                continue;
            } else if (!ghost.released) {
                if (state->releaseTimer > 0.f) {
                    control.speed = 0.f;
                    continue;
                }
                ghost.released = true;
                control.speed = state->frightenedTimer > 0.f
                        ? ghost.baseSpeed * kFrightenedSpeedFactor
                        : ghost.baseSpeed;
                position = cellCenter(ghost.exitX, ghost.exitY, size.width);
                setControlCell(control, ghost.exitX, ghost.exitY);
            }

            int bestDirection = control.direction;
            float bestScore = -std::numeric_limits<float>::infinity();
            int choices = 0;

            for (int direction = 0; direction < 4; direction += 1) {
                const int nextX = normalizeX(control.fromX + tilemap_control_impl::dx(direction));
                const int nextY = control.fromY + tilemap_control_impl::dy(direction);
                if (!isInside(nextX, nextY) || isWall(nextX, nextY)) {
                    continue;
                }
                if (choices > 0 && tilemap_control_impl::isOpposite(control.direction, direction)) {
                    continue;
                }

                const int targetX = ghost.eyes ? ghost.homeX : playerX;
                const int targetY = ghost.eyes ? ghost.homeY : playerY;
                const float score = scoreDirection(
                    control.fromX, control.fromY, direction, targetX, targetY,
                    !ghost.eyes && state->frightenedTimer > 0.f);

                if (score > bestScore) {
                    bestScore = score;
                    bestDirection = direction;
                }
                choices += 1;
            }

            if (choices == 0) {
                for (int direction = 0; direction < 4; direction += 1) {
                    const int nextX = normalizeX(control.fromX + tilemap_control_impl::dx(direction));
                    const int nextY = control.fromY + tilemap_control_impl::dy(direction);
                    if (!isInside(nextX, nextY) || isWall(nextX, nextY)) {
                        continue;
                    }
                    bestDirection = direction;
                    break;
                }
            }

            control.direction = bestDirection;
            control.toX = control.fromX + tilemap_control_impl::dx(control.direction);
            control.toY = control.fromY + tilemap_control_impl::dy(control.direction);
            control.moving = true;
        }
    }
};

SYSTEM(PacmanEnemyCollisionSys, With<PlayerTag, Position, Size>, On<Update>) {
    ITER(view) {
        const auto *hud = view.world.singleton_get<ScoreHudState>();
        const auto *state = view.world.singleton_get<LevelState>();
        if (hud == nullptr || state == nullptr || hud->gameOver) {
            return;
        }

        const auto *playerPositions = view.column<Position>();
        const auto *playerSizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            const ecs::Entity player = view.entity(i);
            const Position playerPosition = playerPositions[i];
            const Size playerSize = playerSizes[i];

            bool lostLife = false;
            view.world.fetch<GhostTag, Position, Size>().iter([&](ArchetypeView &ghostView) {
                const auto *positions = ghostView.column<Position>();
                const auto *sizes = ghostView.column<Size>();
                for (uint32_t ghostIndex = 0; ghostIndex < ghostView.count(); ghostIndex += 1) {
                    if (!overlapsEntity(playerPosition, playerSize, positions[ghostIndex], sizes[ghostIndex])) {
                        continue;
                    }

                    auto *ghost = view.world.get<GhostData>(ghostView.entity(ghostIndex));
                    auto *control = view.world.get<GhostControl>(ghostView.entity(ghostIndex));
                    if (ghost == nullptr || control == nullptr || ghost->eyes) {
                        continue;
                    }

                    if (state->frightenedTimer > 0.f) {
                        ghost->eyes = true;
                        ghost->released = true;
                        ghost->healTimer = 0.f;
                        control->speed = kEyesSpeed;
                        score_plugin_api::addScore(view.world, 200);
                        view.world.set(ghostView.entity(ghostIndex), Color::white());
                        continue;
                    }

                    lostLife = true;
                    break;
                }
            });

            if (!lostLife) {
                continue;
            }

            score_plugin_api::loseLife(view.world);
            resetPlayer(view.world, player);
            auto *mutableState = view.world.singleton_get<LevelState>();
            if (mutableState != nullptr) {
                mutableState->releaseTimer = kGhostReleaseDelay;
                mutableState->frightenedTimer = 0.f;
            }
            view.world.fetch<GhostTag>().iter([&](const ArchetypeView &ghostView) {
                for (uint32_t ghostIndex = 0; ghostIndex < ghostView.count(); ghostIndex += 1) {
                    resetGhost(view.world, ghostView.entity(ghostIndex));
                }
            });

            if (const auto *newHud = view.world.singleton_get<ScoreHudState>(); newHud != nullptr && newHud->gameOver) {
                view.world.set(player, Color{255, 0, 0, 255});
                if (auto *control = view.world.get<TileMapAutoControl>(player); control != nullptr) {
                    control->speed = 0.f;
                    control->moving = false;
                }
            }
        }
    }
};

SYSTEM(PacmanLevelSys, On<Update>) {
    static void run(ecs::World &world) {
        auto *state = world.singleton_get<LevelState>();
        const auto *hud = world.singleton_get<ScoreHudState>();
        if (state == nullptr || hud == nullptr || hud->gameOver || state->pelletsLeft > 0) {
            return;
        }

        state->level += 1;
        state->playerSpeed += 15.f;
        state->ghostSpeed += 10.f;
        state->releaseTimer = kGhostReleaseDelay;
        state->frightenedTimer = 0.f;

        spawnLevelTiles(world);
        applyLevelSpeed(world);
        score_plugin_api::setMessage(world, "NEXT LEVEL", Color{120, 255, 120, 255});

        world.fetch<PlayerTag>().iter([&](const ArchetypeView &view) {
            for (uint32_t i = 0; i < view.count(); i += 1) {
                resetPlayer(world, view.entity(i));
            }
        });
        world.fetch<GhostTag>().iter([&](const ArchetypeView &view) {
            for (uint32_t i = 0; i < view.count(); i += 1) {
                resetGhost(world, view.entity(i));
            }
        });
    }
};

extern "C" IGameModule *load() {
    auto *engine = new Engine("Pacman", [](Engine &engine, [[maybe_unused]] IDisplayModule *api) {
        ecs::World &world = engine.scene<DefaultScene>();

        engine.setScene<DefaultScene>();
        world.plugin<TileMapPlugin>();
        world.plugin<TileMapControlPlugin>();
        world.plugin<GameMenuReturnPlugin>();
        world.plugin<ScorePlugin>(ScorePlugin::Config{
            .font = kHudFont,
            .lives = 3,
            .x = 16.f,
            .y = 16.f,
            .fontSize = 24
        });

        world.registerComponent<PlayerTag>();
        world.registerComponent<GhostTag>();
        world.registerComponent<PelletTag>();
        world.registerComponent<PowerPelletTag>();
        world.registerComponent<LevelTileTag>();
        world.registerComponent<PlayerSpawn>();
        world.registerComponent<GhostData>();
        world.registerComponent<GhostControl>();
        world.singleton_init(new LevelState());

        world.system<PacmanWrapSys>();
        world.system<PacmanPelletSys>();
        world.system<PacmanStateSys>();
        world.system<PacmanGhostAiSys>();
        world.system<PacmanEnemyCollisionSys>();
        world.system<PacmanLevelSys>();

        auto *state = world.singleton_get<LevelState>();
        spawnLevelTiles(world);

        const auto playerPos = findTile('P');
        const ecs::EntityRef player = world.create().add<PlayerTag>().set(
            cellCenter(playerPos[0], playerPos[1], kPlayerSize),
            Size{kPlayerSize, kPlayerSize},
            Color{255, 230, 0, 255},
            PlayerSpawn{playerPos[0], playerPos[1], 1},
            TileMapAutoControl{
                .map = kMap,
                .wall = '#',
                .tileSize = kTileSize,
                .speed = state->playerSpeed,
                .left = Q,
                .right = D,
                .up = Z,
                .down = S,
                .direction = 1,
                .wantedDirection = 1,
                .fromX = playerPos[0],
                .fromY = playerPos[1],
                .toX = playerPos[0],
                .toY = playerPos[1],
                .moving = false,
                .initialized = true
            }
        );

        const std::array<char, 4> ghostTiles = {'1', '2', '3', '4'};
        const std::array<Color, 4> ghostColors = {
            Color{255, 0, 0, 255},
            Color{255, 184, 255, 255},
            Color{0, 255, 255, 255},
            Color{255, 184, 82, 255}
        };

        for (std::size_t i = 0; i < ghostTiles.size(); i += 1) {
            const auto ghostPos = findTile(ghostTiles[i]);
            world.create().add<GhostTag>().set(
                cellCenter(ghostPos[0], ghostPos[1], kPlayerSize),
                Size{kPlayerSize, kPlayerSize},
                ghostColors[i],
                GhostData{
                    .homeX = ghostPos[0],
                    .homeY = ghostPos[1],
                    .exitX = state->releaseX,
                    .exitY = state->releaseY,
                    .direction = static_cast<int>(i % 2 == 0 ? 0 : 1),
                    .baseSpeed = state->ghostSpeed,
                    .baseColor = ghostColors[i],
                    .released = false,
                    .eyes = false,
                    .healTimer = 0.f
                },
                GhostControl{
                    .speed = 0.f,
                    .direction = static_cast<int>(i % 2 == 0 ? 0 : 1),
                    .fromX = ghostPos[0],
                    .fromY = ghostPos[1],
                    .toX = ghostPos[0],
                    .toY = ghostPos[1],
                    .moving = false,
                    .initialized = true
                }
            );
        }

        resetPlayer(world, player.entity());
    }, {
        Resource::font("./assets/Fonts/pixellari.ttf")
    },
    []([[maybe_unused]] Engine &engine, IDisplayModule *api) {
        api->setClearColor({0, 0, 0, 255});
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
