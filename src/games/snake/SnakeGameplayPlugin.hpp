#pragma once
#include <array>
#include <vector>
#include "engine/ecs/World.hpp"
#include "engine/ecs/addons/Timer.hpp"
#include "engine/plugins/score/ScorePlugin.hpp"

struct SnakeLink : ecs::DespawnRelated {
};

struct SnakeBoardTag {
};

struct SnakePartTag {
};

struct SnakeHeadTag {
};

struct SnakeFoodTag {
};

struct SnakeCell : IVec2 {
};

enum class SnakeDirection { Up, Right, Down, Left };

struct SnakeState {
    int width = 21, height = 15;
    float tileSize = 40.f, originX = 160.f, originY = 140.f;
    bool wrap = false, gameOver = false, hasFood = false;
    Timer moveTimer{0.16f};
    SnakeDirection direction = SnakeDirection::Right, wantedDirection = SnakeDirection::Right;
    ecs::Entity head{}, tail{}, food{};
};

namespace snake_impl {
    inline Position pos(const SnakeState &s, const SnakeCell c, const float size) {
        const float inset = (s.tileSize - size) * .5f;
        return {s.originX + static_cast<float>(c.x) * s.tileSize + inset, s.originY + c.y * s.tileSize + inset};
    }

    inline void place(ecs::World &w, const ecs::Entity e, const SnakeCell c) {
        w.set(e, c);
        w.set(e, pos(*w.singleton_get<SnakeState>(), c, w.get<Size>(e)->width));
    }

    inline bool occupied(ecs::World &w, const SnakeCell target, const ecs::Entity ignore = {}) {
        return w.any<SnakePartTag, SnakeCell>([&](ArchetypeView &v, bool &hit) {
            const auto *cells = v.column<SnakeCell>();
            for (uint32_t i = 0; i < v.count(); i += 1)
                if (v.entity(i) != ignore && cells[i].x == target.x && cells[i].y == target.y) {
                    hit = true;
                    break;
                }
        });
    }

    inline SnakeDirection input(ecs::World &w, const SnakeDirection fallback) {
        if (w.api->isKeyPressed(Z) || w.api->isKeyPressed(ArrowUp)) return SnakeDirection::Up;
        if (w.api->isKeyPressed(D) || w.api->isKeyPressed(ArrowRight)) return SnakeDirection::Right;
        if (w.api->isKeyPressed(S) || w.api->isKeyPressed(ArrowDown)) return SnakeDirection::Down;
        if (w.api->isKeyPressed(Q) || w.api->isKeyPressed(ArrowLeft)) return SnakeDirection::Left;
        return fallback;
    }

    inline bool opposite(const SnakeDirection a, const SnakeDirection b) {
        return (a == SnakeDirection::Up && b == SnakeDirection::Down) ||
               (a == SnakeDirection::Down && b == SnakeDirection::Up) ||
               (a == SnakeDirection::Left && b == SnakeDirection::Right) ||
               (a == SnakeDirection::Right && b == SnakeDirection::Left);
    }

    inline SnakeCell next(const SnakeCell c, const SnakeDirection d) {
        if (d == SnakeDirection::Up) return {c.x, c.y - 1};
        if (d == SnakeDirection::Right) return {c.x + 1, c.y};
        if (d == SnakeDirection::Down) return {c.x, c.y + 1};
        return {c.x - 1, c.y};
    }

    inline void lose(ecs::World &w) {
        auto &s = *w.singleton_get<SnakeState>();
        if (!s.gameOver) {
            s.gameOver = true;
            score_plugin_api::loseLife(w);
        }
    }

    inline ecs::Entity spawnPart(ecs::World &w, const SnakeCell c, const ecs::Entity parent, const Color color) {
        const auto &s = *w.singleton_get<SnakeState>();
        const ecs::Entity e = w.create().add<SnakePartTag>().set(
            c, Position{0.f, 0.f}, Size{s.tileSize - 8.f, s.tileSize - 8.f}, color
        ).relate<SnakeLink>(parent).entity();
        place(w, e, c);
        return e;
    }

    inline void spawnFood(ecs::World &w) {
        auto &s = *w.singleton_get<SnakeState>();
        if (s.gameOver || s.hasFood) return;
        std::vector<SnakeCell> free;
        for (int y = 0; y < s.height; y += 1)
            for (int x = 0; x < s.width; x += 1)
                if (!occupied(w, {x, y})) free.push_back({x, y});
        if (free.empty()) {
            s.gameOver = true;
            score_plugin_api::setMessage(w, "YOU WIN", Color{120, 255, 120, 255});
            return;
        }
        s.food = w.create().add<SnakeFoodTag>().set(
            free[std::rand() % free.size()], Position{0.f, 0.f}, Size{s.tileSize - 16.f, s.tileSize - 16.f},
            Color{220, 48, 48, 255}
        ).entity();
        s.hasFood = true;
        place(w, s.food, *w.get<SnakeCell>(s.food));
    }

    inline void setup(ecs::World &w) {
        auto &s = *w.singleton_get<SnakeState>();
        const float width = static_cast<float>(s.width) * s.tileSize, height =
                static_cast<float>(s.height) * s.tileSize;
        const SnakeCell head{s.width / 2, s.height / 2};
        w.create().add<SnakeBoardTag>().set(Position{s.originX - 8.f, s.originY - 8.f},
                                            Size{width + 16.f, height + 16.f}, Color{22, 34, 20, 255});
        w.create().add<SnakeBoardTag>().set(Position{s.originX, s.originY}, Size{width, height},
                                            Color{38, 54, 34, 255});
        s = SnakeState{
            .width = s.width, .height = s.height, .tileSize = s.tileSize, .originX = s.originX, .originY = s.originY,
            .wrap = s.wrap, .moveTimer = s.moveTimer
        };
        s.head = w.create().add<SnakePartTag>().add<SnakeHeadTag>().set(
            head, Position{0.f, 0.f}, Size{s.tileSize - 8.f, s.tileSize - 8.f}, Color{235, 214, 52, 255}
        ).entity();
        place(w, s.head, head);
        ecs::Entity parent = s.head;
        for (int i = 1; i < 4; i += 1) parent = spawnPart(w, {head.x - i, head.y}, parent, Color{122, 196, 82, 255});
        s.tail = parent;
        spawnFood(w);
    }
}

SYSTEM(SnakeInputSys, On<Update>) {
    static void run(ecs::World &w) {
        auto &s = *w.singleton_get<SnakeState>();
        if (s.gameOver || w.api == nullptr) return;
        const SnakeDirection wanted = snake_impl::input(w, s.wantedDirection);
        if (!snake_impl::opposite(s.direction, wanted)) s.wantedDirection = wanted;
    }
};

SYSTEM(SnakeFoodSys, On<Update>) {
    static void run(ecs::World &w) { snake_impl::spawnFood(w); }
};

SYSTEM(SnakeMoveSys, On<Update>) {
    static void run(ecs::World &w) {
        auto &s = *w.singleton_get<SnakeState>();
        if (const auto *hud = w.singleton_get<ScoreHudState>();
            s.gameOver || (hud != nullptr && hud->gameOver) || !s.moveTimer.tick(w.deltaTime))
            return;

        const SnakeCell head = *w.get<SnakeCell>(s.head), tail = *w.get<SnakeCell>(s.tail);
        const SnakeCell food = s.hasFood ? *w.get<SnakeCell>(s.food) : SnakeCell{-1, -1};
        s.direction = s.wantedDirection;
        SnakeCell next = snake_impl::next(head, s.direction);

        if (s.wrap) {
            if (next.x < 0) next.x = s.width - 1;
            else if (next.x >= s.width) next.x = 0;
            if (next.y < 0) next.y = s.height - 1;
            else if (next.y >= s.height) next.y = 0;
        } else if (next.x < 0 || next.x >= s.width || next.y < 0 || next.y >= s.height) return snake_impl::lose(w);

        const bool ate = s.hasFood && next.x == food.x && next.y == food.y;
        if (snake_impl::occupied(w, next) && (ate || next.x != tail.x || next.y != tail.y)) return snake_impl::lose(w);

        SnakeCell prev = head, tailVacated = head;
        for (const auto &[parent, current]: w.iterRelated<SnakeLink, true>(s.head)) {
            (void) parent;
            const SnakeCell cell = *w.get<SnakeCell>(current);
            snake_impl::place(w, current, prev);
            prev = cell;
            tailVacated = cell;
        }
        snake_impl::place(w, s.head, next);
        if (!ate) return;

        w.kill(s.food);
        s.food = {};
        s.hasFood = false;

        SnakeCell grow = tailVacated;
        if (snake_impl::occupied(w, grow)) {
            const SnakeCell base = *w.get<SnakeCell>(s.tail);
            constexpr std::array<SnakeCell, 4> dirs{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
            bool found = false;
            for (const auto d: dirs) {
                grow = {base.x + d.x, base.y + d.y};
                if (grow.x >= 0 && grow.x < s.width && grow.y >= 0 && grow.y < s.height && !
                    snake_impl::occupied(w, grow)) {
                    found = true;
                    break;
                }
            }
            if (!found) return snake_impl::lose(w);
        }

        s.tail = snake_impl::spawnPart(w, grow, s.tail, Color{122, 196, 82, 255});
        score_plugin_api::addScore(w, 100);
        snake_impl::spawnFood(w);
    }
};

SYSTEM(SnakeHudSys, On<Update>) {
    static void run(ecs::World &w) {
        if (const auto *hud = w.singleton_get<ScoreHudState>(); hud != nullptr && hud->gameOver)
            w.singleton_get<SnakeState>()->gameOver = true;
    }
};

struct SnakeGameplayPlugin {
    struct Config {
        int width = 21, height = 15;
        float tileSize = 40.f, originX = 160.f, originY = 140.f, moveInterval = 0.16f;
        bool wrap = false;
    };

    Config config;

    SnakeGameplayPlugin() = default;

    explicit SnakeGameplayPlugin(const Config &config) : config(config) {
    }

    void load(ecs::World &w) {
        w.registerComponent<SnakeBoardTag>();
        w.registerComponent<SnakePartTag>();
        w.registerComponent<SnakeHeadTag>();
        w.registerComponent<SnakeFoodTag>();
        w.registerComponent<SnakeCell>();
        w.relation<SnakeLink>();
        auto *s = new SnakeState();
        s->width = config.width;
        s->height = config.height;
        s->tileSize = config.tileSize;
        s->originX = config.originX;
        s->originY = config.originY;
        s->wrap = config.wrap;
        s->moveTimer = Timer{config.moveInterval};
        w.singleton_init(s);
        snake_impl::setup(w);
        w.system<SnakeInputSys>();
        w.system<SnakeMoveSys>();
        w.system<SnakeFoodSys>();
        w.system<SnakeHudSys>();
    }

    void unload(ecs::World &w) {
        w.remove<SnakeInputSys>();
        w.remove<SnakeMoveSys>();
        w.remove<SnakeFoodSys>();
        w.remove<SnakeHudSys>();
        w.singleton_remove<SnakeState>();
    }
};
