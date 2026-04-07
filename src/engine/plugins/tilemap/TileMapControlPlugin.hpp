#pragma once

#include <cstdlib>
#include <string_view>

#include "TileMapPlugin.hpp"
#include "engine/ecs/addons/Timer.hpp"

struct TileMapAutoControl : Required<Position, Size> {
    const char *map = "";
    char wall = '#';
    float tileSize = 64.f;
    float speed = 220.f;
    KeyboardCode left = Q;
    KeyboardCode right = D;
    KeyboardCode up = Z;
    KeyboardCode down = S;
    int direction = 1;
    int wantedDirection = 1;
    int fromX = 0;
    int fromY = 0;
    int toX = 0;
    int toY = 0;
    bool moving = false;
    bool initialized = false;
};

struct TileMapRandomControl : Required<Position, Size> {
    const char *map = "";
    char wall = '#';
    float tileSize = 64.f;
    float speed = 170.f;
    int direction = 0;
    int fromX = 0;
    int fromY = 0;
    int toX = 0;
    int toY = 0;
    bool moving = false;
    bool initialized = false;
    Timer changeTimer{0.35f};
};

namespace tilemap_control_impl {
    inline int dx(const int direction) {
        if (direction == 0) return -1;
        if (direction == 1) return 1;
        return 0;
    }

    inline int dy(const int direction) {
        if (direction == 2) return -1;
        if (direction == 3) return 1;
        return 0;
    }

    inline bool isOpposite(const int lhs, const int rhs) {
        return (lhs == 0 && rhs == 1) || (lhs == 1 && rhs == 0) || (lhs == 2 && rhs == 3) || (lhs == 3 && rhs == 2);
    }

    inline int cellX(const Position &position, const Size &size, const float tileSize) {
        return static_cast<int>((position.x + size.width * 0.5f) / tileSize);
    }

    inline int cellY(const Position &position, const Size &size, const float tileSize) {
        return static_cast<int>((position.y + size.height * 0.5f) / tileSize);
    }

    inline float cellPosition(const int cell, const float size, const float tileSize) {
        return static_cast<float>(cell) * tileSize + (tileSize - size) * 0.5f;
    }

    inline Position cellCenter(const int x, const int y, const Size &size, const float tileSize) {
        return {cellPosition(x, size.width, tileSize), cellPosition(y, size.height, tileSize)};
    }

    inline bool isWall(const std::string_view map, const char wall, const int x, const int y) {
        const TileMapGrid grid = TileMapPlugin::toGrid(map);
        return TileMapPlugin::isTile(grid, x, y, wall);
    }

    inline bool canMove(const int x, const int y, const int direction, const std::string_view map, const char wall) {
        return !isWall(map, wall, x + dx(direction), y + dy(direction));
    }

    inline void init(Position &position,
                     const Size &size,
                     const float tileSize,
                     int &fromX,
                     int &fromY,
                     int &toX,
                     int &toY,
                     bool &initialized) {
        if (initialized) {
            return;
        }
        fromX = cellX(position, size, tileSize);
        fromY = cellY(position, size, tileSize);
        toX = fromX;
        toY = fromY;
        position = cellCenter(fromX, fromY, size, tileSize);
        initialized = true;
    }

    inline bool moveToward(Position &position, const Position &target, const float speed, const float dt) {
        const float step = speed * dt;

        if (position.x < target.x) {
            position.x = position.x + step >= target.x ? target.x : position.x + step;
        } else if (position.x > target.x) {
            position.x = position.x - step <= target.x ? target.x : position.x - step;
        }

        if (position.y < target.y) {
            position.y = position.y + step >= target.y ? target.y : position.y + step;
        } else if (position.y > target.y) {
            position.y = position.y - step <= target.y ? target.y : position.y - step;
        }

        return position.x == target.x && position.y == target.y;
    }
}

SYSTEM(TileMapAutoControlSys, With<TileMapAutoControl, Position, Size>, On<Update>) {
    ITER(view) {
        if (view.world.api == nullptr) {
            return;
        }

        auto *controls = view.column<TileMapAutoControl>();
        auto *positions = view.column<Position>();
        auto *sizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            auto &control = controls[i];
            auto &position = positions[i];
            const Size &size = sizes[i];
            const std::string_view map = control.map;

            tilemap_control_impl::init(
                position, size, control.tileSize,
                control.fromX, control.fromY, control.toX, control.toY, control.initialized);

            if (view.world.api->isKeyPressed(control.left) || view.world.api->isKeyPressed(ArrowLeft)) {
                control.wantedDirection = 0;
            } else if (view.world.api->isKeyPressed(control.right) || view.world.api->isKeyPressed(ArrowRight)) {
                control.wantedDirection = 1;
            } else if (view.world.api->isKeyPressed(control.up) || view.world.api->isKeyPressed(ArrowUp)) {
                control.wantedDirection = 2;
            } else if (view.world.api->isKeyPressed(control.down) || view.world.api->isKeyPressed(ArrowDown)) {
                control.wantedDirection = 3;
            }

            if (control.moving) {
                if (tilemap_control_impl::isOpposite(control.direction, control.wantedDirection)) {
                    std::swap(control.fromX, control.toX);
                    std::swap(control.fromY, control.toY);
                    control.direction = control.wantedDirection;
                }

                const Position target = tilemap_control_impl::cellCenter(control.toX, control.toY, size, control.tileSize);
                if (tilemap_control_impl::moveToward(position, target, control.speed, view.world.deltaTime)) {
                    control.fromX = control.toX;
                    control.fromY = control.toY;
                    control.moving = false;
                }
                continue;
            }

            int nextDirection = control.direction;
            if (tilemap_control_impl::canMove(control.fromX, control.fromY, control.wantedDirection, map, control.wall)) {
                nextDirection = control.wantedDirection;
            } else if (!tilemap_control_impl::canMove(control.fromX, control.fromY, nextDirection, map, control.wall)) {
                continue;
            }

            control.direction = nextDirection;
            control.toX = control.fromX + tilemap_control_impl::dx(control.direction);
            control.toY = control.fromY + tilemap_control_impl::dy(control.direction);
            control.moving = true;
        }
    }
};

SYSTEM(TileMapRandomControlSys, With<TileMapRandomControl, Position, Size>, On<Update>) {
    ITER(view) {
        auto *controls = view.column<TileMapRandomControl>();
        auto *positions = view.column<Position>();
        auto *sizes = view.column<Size>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            auto &control = controls[i];
            auto &position = positions[i];
            const Size &size = sizes[i];
            const std::string_view map = control.map;

            tilemap_control_impl::init(
                position, size, control.tileSize,
                control.fromX, control.fromY, control.toX, control.toY, control.initialized);

            if (control.moving) {
                const Position target = tilemap_control_impl::cellCenter(control.toX, control.toY, size, control.tileSize);
                if (tilemap_control_impl::moveToward(position, target, control.speed, view.world.deltaTime)) {
                    control.fromX = control.toX;
                    control.fromY = control.toY;
                    control.moving = false;
                }
                continue;
            }

            int directions[4];
            int count = 0;
            for (int direction = 0; direction < 4; direction += 1) {
                if (!tilemap_control_impl::canMove(control.fromX, control.fromY, direction, map, control.wall)) {
                    continue;
                }
                directions[count] = direction;
                count += 1;
            }

            if (count == 0) {
                continue;
            }

            if (!tilemap_control_impl::canMove(control.fromX, control.fromY, control.direction, map, control.wall) ||
                control.changeTimer.tick(view.world.deltaTime)) {
                control.direction = directions[std::rand() % count];
            }

            control.toX = control.fromX + tilemap_control_impl::dx(control.direction);
            control.toY = control.fromY + tilemap_control_impl::dy(control.direction);
            control.moving = true;
        }
    }
};

struct TileMapControlPlugin {
    void load(ecs::World &world) {
        world.registerComponent<TileMapAutoControl>();
        world.registerComponent<TileMapRandomControl>();
        world.system<TileMapAutoControlSys>();
        world.system<TileMapRandomControlSys>();
    }

    void unload(ecs::World &world) {
        world.remove<TileMapAutoControlSys>();
        world.remove<TileMapRandomControlSys>();
    }
};
