#pragma once

#include "engine/Engine.hpp"

struct GameMenuReturnState {
    bool escapeHeld = false;
};

SYSTEM(ReturnToMenuOnEscapeSys, On<Update>) {
    static void run(ecs::World &world) {
        if (world.api == nullptr) {
            return;
        }

        auto *state = world.singleton_get<GameMenuReturnState>();
        auto *engine = world.singleton_get<Engine>();
        if (state == nullptr || engine == nullptr) {
            return;
        }

        const bool escapePressed = world.api->isKeyPressed(Escape);
        if (escapePressed && !state->escapeHeld) {
            engine->pendingActions.push_back(CoreAction{
                .type = CoreActionType::SwitchGame,
                .target = "./lib/arcade_menu.so"
            });
        }
        state->escapeHeld = escapePressed;
    }
};

struct GameMenuReturnPlugin {
    void load(ecs::World &world) {
        world.singleton_init<GameMenuReturnState>();
        world.system<ReturnToMenuOnEscapeSys>();
    }

    void unload(ecs::World &world) {
        world.remove<ReturnToMenuOnEscapeSys>();
        world.singleton_remove<GameMenuReturnState>();
    }
};