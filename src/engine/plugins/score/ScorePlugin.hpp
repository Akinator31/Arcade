#pragma once

#include <cstdlib>
#include <string>

#include "engine/ecs/World.hpp"
#include "engine/plugins/render/UiPlugin/UiPlugin.hpp"

struct AddScoreEvent {
    uint32_t value = 0;
};

struct LoseLifeEvent {
    uint32_t value = 1;
};

struct SetHudMessageEvent {
    const char *message = "";
    Color color = Color::white();
};

struct ClearHudMessageEvent {
};

struct ScoreHudState {
    uint32_t score = 0;
    uint32_t lives = 3;
    bool gameOver = false;
    std::string message;
    Color messageColor = Color::white();
    ecs::Entity textEntity{};
    ResourceIndex font = 0;
    float x = 16.f;
    float y = 16.f;
    uint32_t fontSize = 28;
    bool dirty = true;
};

namespace score_plugin_api {
    inline void addScore(ecs::World &world, const uint32_t value) {
        world.globalEmit(AddScoreEvent{value});
    }

    inline void loseLife(ecs::World &world, const uint32_t value = 1) {
        world.globalEmit(LoseLifeEvent{value});
    }

    inline void setMessage(ecs::World &world, const char *message, const Color color = Color::white()) {
        world.globalEmit(SetHudMessageEvent{message, color});
    }

    inline void clearMessage(ecs::World &world) {
        world.globalEmit(ClearHudMessageEvent{});
    }
}

SYSTEM(ScoreHudSyncSys, On<Update>) {
    static void run(ecs::World &world) {
        auto *state = world.singleton_get<ScoreHudState>();
        if (state == nullptr || !state->dirty || !world.isAlive(state->textEntity)) {
            return;
        }

        std::string text = "SCORE " + std::to_string(state->score) + "   LIVES " + std::to_string(state->lives);
        if (!state->message.empty()) {
            text += "   ";
            text += state->message;
        }

        world.set(state->textEntity, Text{
            state->font,
            strdup(text.c_str()),
            state->message.empty() ? Color::white() : state->messageColor,
            state->fontSize
        });
        state->dirty = false;
    }
};

struct ScorePlugin {
    struct Config {
        ResourceIndex font = 0;
        uint32_t lives = 3;
        float x = 16.f;
        float y = 16.f;
        uint32_t fontSize = 28;
    };

    Config config;
    EventListenerId scoreListener = 0;
    EventListenerId lifeListener = 0;
    EventListenerId messageListener = 0;
    EventListenerId clearListener = 0;

    ScorePlugin() = default;

    explicit ScorePlugin(const Config &config) : config(config) {
    }

    void load(ecs::World &world) {
        world.registerComponent<Text>();
        auto *state = new ScoreHudState();
        state->font = config.font;
        state->lives = config.lives;
        state->x = config.x;
        state->y = config.y;
        state->fontSize = config.fontSize;
        world.singleton_init<ScoreHudState>(state);

        state->textEntity = world.create().set(
            Position{state->x, state->y},
            Text{state->font, strdup(""), Color::white(), state->fontSize}
        ).entity();

        scoreListener = world.globalListen<AddScoreEvent>([](ecs::World &w, const AddScoreEvent &event) {
            auto *state = w.singleton_get<ScoreHudState>();
            if (state == nullptr || state->gameOver) {
                return;
            }
            state->score += event.value;
            state->dirty = true;
        });

        lifeListener = world.globalListen<LoseLifeEvent>([](ecs::World &w, const LoseLifeEvent &event) {
            auto *state = w.singleton_get<ScoreHudState>();
            if (state == nullptr || state->gameOver) {
                return;
            }

            state->lives = event.value >= state->lives ? 0 : state->lives - event.value;
            if (state->lives == 0) {
                state->gameOver = true;
                state->message = "GAME OVER";
                state->messageColor = Color::red();
            } else {
                state->message = "OUCH";
                state->messageColor = Color{255, 180, 0, 255};
            }
            state->dirty = true;
        });

        messageListener = world.globalListen<SetHudMessageEvent>([](ecs::World &w, const SetHudMessageEvent &event) {
            auto *state = w.singleton_get<ScoreHudState>();
            if (state == nullptr) {
                return;
            }
            state->message = event.message == nullptr ? "" : event.message;
            state->messageColor = event.color;
            state->dirty = true;
        });

        clearListener = world.globalListen<ClearHudMessageEvent>([](ecs::World &w, const ClearHudMessageEvent &) {
            auto *state = w.singleton_get<ScoreHudState>();
            if (state == nullptr || state->gameOver) {
                return;
            }
            state->message.clear();
            state->messageColor = Color::white();
            state->dirty = true;
        });

        world.system<ScoreHudSyncSys>();
    }

    void unload(ecs::World &world) {
        world.globalUnlisten<AddScoreEvent>(scoreListener);
        world.globalUnlisten<LoseLifeEvent>(lifeListener);
        world.globalUnlisten<SetHudMessageEvent>(messageListener);
        world.globalUnlisten<ClearHudMessageEvent>(clearListener);
        if (auto *state = world.singleton_get<ScoreHudState>(); state != nullptr && world.isAlive(state->textEntity)) {
            world.kill(state->textEntity);
        }
        world.singleton_remove<ScoreHudState>();
        world.remove<ScoreHudSyncSys>();
    }
};
