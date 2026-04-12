# ECS Systems

This guide covers batch systems, stateful systems, observers, phases, and run conditions.

## 1. System forms

ECS supports three common forms.

### Batch system with `iter`

Most systems are batch systems. They run on every table matching `With<>` and `Without<>`.

```cpp
struct MoveSystem : With<Position, Velocity>, On<Update> {
    static void iter(ArchetypeView &view) {
        auto *positions = view.column<Position>();
        auto *velocities = view.column<Velocity>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            positions[i].x += velocities[i].x;
            positions[i].y += velocities[i].y;
        }
    }
};
```

### Stateful system with `run`

Use this when the system must keep internal state between frames.

```cpp
struct SpawnTimer {
    float accumulator = 0.f;

    void run(ecs::World &world) {
        accumulator += world.deltaTime;
        if (accumulator >= 1.f) {
            accumulator = 0.f;
            world.create().set(Name{"enemy"});
        }
    }
};
```

The world keeps the system instance alive while it stays registered.

### Mixed system

A system can implement both `iter` and `run`:

- `iter` handles matched entity batches
- `run` handles world-level orchestration

## 2. Macro helpers

Available macros:

- `SYSTEM(Name, ...)`
- `ITER(view)`
- `RUN(world)`
- `OBSERVE(table, row)`

Example:

```cpp
SYSTEM(GravitySys, With<Velocity, Gravity>, On<PostUpdate>) {
    ITER(view) {
        auto *velocities = view.column<Velocity>();
        const auto *gravity = view.column<Gravity>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            velocities[i].y += gravity[i].scale * view.world.deltaTime;
        }
    }
};
```

## 3. Entity selection

### `With`

All listed components must exist.

```cpp
struct RenderRects : With<GlobalPosition, Size, Color> { };
```

### `Without`

Exclude entities that carry some components.

```cpp
struct MoveOnlyFreeBodies : With<Position, Velocity>, Without<RigidBody> { };
```

### Access in `ArchetypeView`

- `view.count()`
- `view.entity(i)`
- `view.entities()`
- `view.column<T>()`
- `view.optional<T>()`
- `view.has<T>()`
- `view.world`

Important:

- `column<T>()` assumes `T` is required by the query
- `optional<T>()` returns `nullptr` when `T` is missing in the current table
- iteration is archetype-by-archetype, not top-level entity-by-entity

## 4. Phases

By default, a system is added to `Update`.

To pick a phase:

```cpp
struct MySys : With<Position>, On<PreRender> {
    static void iter(ArchetypeView &view) { }
};
```

Available phases:

- `PreStartup`
- `Startup`
- `PostStartup`
- `PreUpdate`
- `Update`
- `PostUpdate`
- `PreRender`
- `Render`

In `World::progress()`, runtime phases are:

- `PreUpdate`
- `Update`
- `PostUpdate`
- `PreRender`
- `Render`

`PostStartup` exists as a phase type, but it is not executed by `start()` or `progress()` in the current implementation.

## 5. Conditions

A system can expose a static condition.

```cpp
struct OnlyWhenPaused {
    static bool condition(ecs::World &world) {
        return world.getState<GameState>() == GameState::Paused;
    }
};

struct PauseMenuSys : With<UiTag>, OnlyWhenPaused {
    static void iter(ArchetypeView &view) { }
};
```

Compose multiple conditions:

```cpp
struct Sys : With<Position>, Conditions<A, B> {
    static void iter(ArchetypeView &view) { }
};
```

Built-in helpers:

- `InState<VALUE>`: run only if `world.getState<decltype(VALUE)>() == VALUE`
- `Interval<MS>`: run periodically using `world.deltaTime`

## 6. Register and remove systems

```cpp
const SystemId id = world.system<MoveSystem>();
world.runSystem(id);
world.remove<MoveSystem>();
```

Notes:

- `world.system<T>()` constructs an instance when `T()` or `T(world)` is available
- `world.remove<T>()` removes all registered occurrences of this system type
- `runSystem(id)` executes one system directly, without condition checks and without automatic `flush()`
- `progress()` runs all systems in runtime phases

## 7. Observers and `OBSERVE`

Observers do not use `iter`; they use `observe`.
The `OBSERVE(table, row)` macro expands to:

```cpp
static void observe(ecs::internal::Archetype &table, ecs::internal::EntityRow row)
```

Use the macro inside a `SYSTEM(...)` declaration to keep syntax consistent.

### On-add observer

```cpp
SYSTEM(OnAddPlayer, With<Player>, On<Add>) {
    OBSERVE(table, row) {
        (void)table;
        (void)row;
        // Triggered when an entity enters a table containing Player
    }
};
```

### On-remove observer

```cpp
SYSTEM(OnRemovePlayer, With<Player>, On<Remove>) {
    OBSERVE(table, row) {
        (void)table;
        (void)row;
    }
};
```

### On-despawn observer

```cpp
SYSTEM(OnDespawnPlayer, With<Player>, On<Despawn>) {
    OBSERVE(table, row) {
        (void)table;
        (void)row;
    }
};
```

Behavior details:

- observers registered later still attach to already existing matching tables
- `On<Remove>` hooks into the required component column `onRemove` callbacks
- `On<Despawn>` is triggered by `world.kill(entity)`

## 8. Deferred commands

When system logic mutates world structure during iteration, defer it:

```cpp
view.world.command([entity = view.entity(i)](ecs::World &world) {
    world.add<HoveredComponent>(entity);
});
```

Queued commands are flushed after each system in `runAll()`.

Use this for:

- adding/removing components while iterating
- creating or killing entities from batch systems
- structural updates that would invalidate current iteration

## 9. Practical advice

- prefer `iter` for data-oriented processing
- prefer `run` for orchestration and timers
- keep observers small and event-focused
- use explicit phases and conditions to avoid hidden execution order issues