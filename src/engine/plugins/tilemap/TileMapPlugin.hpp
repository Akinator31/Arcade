#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "engine/ecs/World.hpp"

using TileMapGrid = std::vector<std::string>;

struct TileMapOptions {
    std::string_view map;
    char tile = '#';
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

    static bool isTile(const TileMapGrid &map, const int x, const int y, const char tile) {
        return y >= 0 && y < static_cast<int>(map.size()) &&
               x >= 0 && x < static_cast<int>(map[y].size()) &&
               map[y][x] == tile;
    }

    static void spawnTile(const ecs::EntityRef &base,
                          const Position base_position,
                          const Size base_size,
                          const int x,
                          const int y) {
        auto tile = base.clone();
        tile.set(
            Position{
                base_position.x + static_cast<float>(x) * base_size.width,
                base_position.y + static_cast<float>(y) * base_size.height
            },
            base_size
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

        for (int y = 0; y < static_cast<int>(map.size()); y += 1) {
            for (int x = 0; x < static_cast<int>(map[y].size()); x += 1) {
                if (!isTile(map, x, y, options.tile)) {
                    continue;
                }
                spawnTile(base, origin, tile_size, x, y);
            }
        }

        world.kill(base.entity());
    }

    void load(ecs::World &) {
    }

    void unload(ecs::World &) {
    }
};
