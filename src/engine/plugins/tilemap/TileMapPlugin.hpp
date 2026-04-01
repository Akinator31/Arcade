#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "engine/ecs/World.hpp"

using TileMapGrid = std::vector<std::string>;

struct TileMapOptions {
    std::string_view map;
    char wall = '#';
};

struct TileMapPlugin {
    static TileMapGrid toGrid(const std::string_view ascii_map) {
        TileMapGrid rows;
        std::string current_row;

        for (const char c: ascii_map) {
            if (c == '\n') {
                if (!current_row.empty()) {
                    rows.push_back(current_row);
                    current_row.clear();
                }
                continue;
            }
            current_row.push_back(c);
        }

        if (!current_row.empty()) {
            rows.push_back(current_row);
        }

        std::size_t width = 0;
        for (const auto &row: rows) {
            width = std::max(width, row.size());
        }

        for (auto &row: rows) {
            row.resize(width, ' ');
        }

        return rows;
    }

    static bool isWall(const TileMapGrid &map, const int x, const int y, const char wall) {
        return y >= 0 && y < static_cast<int>(map.size()) &&
               x >= 0 && x < static_cast<int>(map[y].size()) &&
               map[y][x] == wall;
    }

    static void spawnWall(const ecs::EntityRef &base,
                          const Position base_position,
                          const Size base_size,
                          const int x,
                          const int y,
                          const int width,
                          const int height) {
        auto wall = base.clone();
        wall.set(
            Position{
                base_position.x + static_cast<float>(x) * base_size.width,
                base_position.y + static_cast<float>(y) * base_size.height
            },
            Size{
                static_cast<float>(width) * base_size.width,
                static_cast<float>(height) * base_size.height
            }
        );
    }

    static void spawn(const ecs::EntityRef &base, const TileMapOptions &options) {
        ecs::World &world = base.world;
        const auto *base_position = world.get<Position>(base.entity());
        const auto *base_size = world.get<Size>(base.entity());
        if (base_position == nullptr || base_size == nullptr) {
            return;
        }

        const Position origin = *base_position;
        const Size tile_size = *base_size;
        const TileMapGrid map = toGrid(options.map);
        if (map.empty()) {
            return;
        }

        std::vector visited(map.size(), std::vector<bool>(map.front().size(), false));

        for (int y = 0; y < static_cast<int>(map.size()); y += 1) {
            for (int x = 0; x < static_cast<int>(map[y].size()); x += 1) {
                if (!isWall(map, x, y, options.wall) || visited[y][x]) {
                    continue;
                }

                int width = 1;
                while (isWall(map, x + width, y, options.wall) && !visited[y][x + width]) {
                    width += 1;
                }

                int height = 1;
                bool can_extend = true;
                while (can_extend && y + height < static_cast<int>(map.size())) {
                    for (int dx = 0; dx < width; dx += 1) {
                        if (!isWall(map, x + dx, y + height, options.wall) || visited[y + height][x + dx]) {
                            can_extend = false;
                            break;
                        }
                    }
                    if (can_extend) {
                        height += 1;
                    }
                }

                for (int dy = 0; dy < height; dy += 1) {
                    for (int dx = 0; dx < width; dx += 1) {
                        visited[y + dy][x + dx] = true;
                    }
                }

                spawnWall(base, origin, tile_size, x, y, width, height);
            }
        }

        world.kill(base.entity());
    }

    void load(ecs::World &) {
    }

    void unload(ecs::World &) {
    }
};
