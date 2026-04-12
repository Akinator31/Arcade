# ECS Quickstart

This quickstart shows the fastest path to build a game scene with `Engine` and `ecs::World`.

## 1. Mental model

- `Engine` owns one or more scenes
- each scene is an `ecs::World`
- a `World` contains entities, components, systems, plugins, states, and singletons
- `engine.update(api)` updates `world.api`, `world.deltaTime`, then runs `world.progress()`

## 2. Core concepts

- an entity is a lightweight identifier
- a component is a plain data struct
- a system executes logic on entities matched by a query
- a plugin is a reusable bundle that registers components/systems/singletons

## 3. First world

```cpp
#include "engine/Engine.hpp"

struct MainScene;

struct PlayerTag {};

SYSTEM(MovePlayer, With<Position, Velocity, PlayerTag>, On<Update>) {
    ITER(view) {
        auto *positions = view.column<Position>();
        auto *velocities = view.column<Velocity>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            positions[i].x += velocities[i].x * view.world.deltaTime;
            positions[i].y += velocities[i].y * view.world.deltaTime;
        }
    }
};

extern "C" IGameModule *load() {
    return reinterpret_cast<IGameModule *>(new Engine("MyGame", [](Engine &engine, IDisplayModule *api) {
        (void)api;
        auto &world = engine.scene<MainScene>();
        engine.setScene<MainScene>();

        world.registerComponent<PlayerTag>();
        world.registerComponent<GlobalPosition>();
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        world.system<MovePlayer>();

        world.create()
            .add<PlayerTag>()
            .set(Position{0.f, 0.f}, Velocity{120.f, 0.f}, Name{"player"});
    }, {}));
}
```

## 4. Frame cycle

`world.progress()` runs phases in this order:

1. `PreUpdate`
2. `Update`
3. `PostUpdate`
4. `PreRender`
5. `Render`

`world.start()` runs:

1. `PreStartup`
2. `Startup`
3. `PreUpdate`

## 5. Useful API right away

Creation:

```cpp
ecs::Entity e = world.entity();
ecs::EntityRef ref = world.create();
```

Add or write:

```cpp
world.add<Position, Velocity>(e);
world.set<Position>(e, {10.f, 20.f});
world.set<Velocity>(e, {1.f, 0.f});
```

Read:

```cpp
if (world.has<Position>(e)) {
    auto *pos = world.get<Position>(e);
}
```

Remove:

```cpp
world.remove<Velocity>(e);
world.kill(e);
```

Lookup by name:

```cpp
auto found = world.findEntityByName("player");
```

## 6. Recommended style

- register gameplay components explicitly
- register all required components used by gameplay components
- use `EntityRef` for fluent entity setup
- keep data in components and behavior in systems
- reserve singletons and typed `state` for world-level context
- group reusable features into plugins

## 7. Read next

- [ECS Systems](ECS_SYSTEMS.md)
- [ECS Components](ECS_COMPONENTS.md)
- [ECS Plugins](ECS_PLUGINS.md)