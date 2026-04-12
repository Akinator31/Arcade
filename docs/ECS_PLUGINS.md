# ECS Plugins

A plugin packages a reusable ECS feature.

## 1. Interface

A plugin is any type exposing:

```cpp
struct MyPlugin {
    void load(ecs::World &world);
    void unload(ecs::World &world);
};
```

`World` then treats it as a valid plugin type.

## 2. Loading

```cpp
world.plugin<MyPlugin>();
world.plugin<StatefulPlugin>(arg1, arg2);
```

Behavior:

- a plugin is instantiated once per type
- constructor arguments are forwarded
- `load(world)` runs immediately
- `world.hasPlugin<T>()` checks availability
- `world.getPlugin<T>()` returns the instance

## 3. Unloading

```cpp
world.removePlugin<MyPlugin>();
```

Behavior:

- `unload(world)` is called
- instance is destroyed
- `World::~World()` also unloads remaining plugins automatically

## 4. Minimal example

```cpp
struct GameplayPlugin {
    void load(ecs::World &world) {
        world.registerComponent<Player>();
        world.registerComponent<Enemy>();
        world.system<MovePlayer>();
        world.system<DamageEnemy>();
    }

    void unload(ecs::World &world) {
        world.remove<MovePlayer>();
        world.remove<DamageEnemy>();
    }
};
```

## 5. What a good plugin contains

- registration of feature components
- related systems and observers
- singletons used by the feature
- initial state values when required
- relation setup if needed
- explicit teardown in `unload`

## 6. Repository examples

### `RenderPlugin`

Loads:

- `PositionPropagationPlugin`
- `Size` and `Color` components
- rectangle and sprite rendering systems

### `SpritePlugin`

Loads:

- `Sprite`, `SpriteAnimation`, `SpriteAtlas`
- `SpriteAnimationSys`

### `PhysicsPlugin`

Loads:

- physics components
- integration and collision systems
- collision and spatial-query singletons

### `DefaultPlugin`

Assembles base plugins for fast project bootstrapping.

## 7. Plugins at `Engine` scene level

An `Engine` scene can be created from a plugin type:

```cpp
struct MainScenePlugin {
    void load(ecs::World &world) {
        world.plugin<DefaultPlugin>();
        world.system<MainGameplay>();
    }

    void unload(ecs::World &world) {
        world.remove<MainGameplay>();
    }
};

auto &scene = engine.scene<MainScenePlugin>();
```

In `Engine::scene<Scene>()`, if `Scene` satisfies `IsPlugin`, the plugin is loaded when the scene is first created.

## 8. Recommendations

- keep plugin usage idempotent through `world.plugin<T>()`
- centralize feature dependencies inside the plugin
- remove systems explicitly in `unload`
- clean singleton data in `unload`
- avoid hidden ordering dependencies when possible

## 9. Common pitfalls

- `registerComponent<T>()` is no-op when already registered, so repeated calls are tolerated
- `world.plugin<T>()` does not reload if plugin is already present
- plugins creating entities in `load()` should define ownership and cleanup strategy