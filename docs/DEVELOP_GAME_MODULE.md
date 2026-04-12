# Develop a Game Module

This guide explains how to add a new game shared library (`.so`) that can be loaded by the Arcade core.

## 1. Understand the contract

A game library must expose two C symbols:

```cpp
extern "C" IGameModule *load();
extern "C" void unload(IGameModule *game);
```

The core resolves these symbols through `GameLoader` (`src/core/dynamic/GameLoader.hpp`).

## 2. Start from a minimal template

A practical base is `src/games/example/ExampleGame.cpp`.

Minimal shape:

```cpp
#include "engine/Engine.hpp"
#include "IGameModule.hpp"

struct MainScene;

extern "C" IGameModule *load() {
    auto *engine = new Engine("MyGame", [](Engine &engine, IDisplayModule *api) {
        (void)api;
        auto &world = engine.scene<MainScene>();
        engine.setScene<MainScene>();

        world.registerComponent<GlobalPosition>();
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        // Register systems and create entities here.
    }, {
        // Optional resources to preload in graphics modules.
    });

    return reinterpret_cast<IGameModule *>(engine);
}

extern "C" void unload(IGameModule *game) {
    delete game;
}
```

## 3. Register your target in CMake

In `CMakeLists.txt`, add a shared library in the `if (BUILD_GAMES)` block.

Example pattern:

```cmake
add_library(my_game SHARED
    src/games/mygame/MyGame.cpp
)
target_sources(my_game PRIVATE $<TARGET_OBJECTS:arcade_ecs>)
target_include_directories(my_game PRIVATE src src/games/mygame)
target_link_libraries(my_game PRIVATE arcade_shared arcade_shared_iface)
set_target_properties(my_game PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME arcade_game_mygame
    PREFIX ""
)
```

The core expects loadable `.so` files from `build/lib/`.

## 4. Build gameplay with ECS

Recommended approach:

- define data in components
- define behavior in systems (`SYSTEM`, `ITER`, `RUN`)
- group reusable feature sets into plugins
- use typed state/singletons for world-level context

See also:

- [ECS Quickstart](ECS_QUICKSTART.md)
- [ECS Systems](ECS_SYSTEMS.md)
- [ECS Components](ECS_COMPONENTS.md)
- [ECS Plugins](ECS_PLUGINS.md)

## 5. Trigger game and graphics switches

A game can request core actions through the `IGameModule` contract implemented by `Engine`.

In this repository, actions are queued via `Engine::pendingActions` and consumed in `src/main.cpp`.

Typical use cases:

- switch to another game `.so`
- switch graphics backend `.so`
- quit the application

## 6. Resource loading

Return resources from `Engine` constructor (third parameter).
They are forwarded to graphics modules with `api->loadResources(game->getResources())`.

Common resource categories:

- textures
- fonts

Use project-relative paths that exist in runtime context (for example under `assets/`).

## 7. Validation checklist

- exported symbols are exactly `load` and `unload`
- library is generated in `build/lib/`
- module can be selected by the menu/core action flow
- no per-frame leaks when switching away from the game
- resources are available in every graphics backend you target