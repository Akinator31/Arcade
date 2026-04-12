# ECS Component Hooks

ECS lets you attach behavior to the component lifecycle.

## 1. Available hooks

A component can define:

- `static void onAdd(ecs::World &, ecs::Entity)`
- `static void onRemove(ecs::World &, ecs::Entity, const T *)`
- `static void onSet(ecs::World &, ecs::Entity, const T *)`
- `static auto required_components()`

A component can also expose constructors:

- `T()`
- `T(ecs::World &)`

## 2. `onAdd`

Called right after a component is added.

Real example: `Name::onAdd()` synchronizes name indexing in `World`.

```cpp
struct SpawnedAt {
    float time = 0.f;

    static void onAdd(ecs::World &world, ecs::Entity entity) {
        world.get<SpawnedAt>(entity)->time = world.deltaTime;
    }
};
```

## 3. `onRemove`

Called before a value is removed from the world.

It runs for:

- `world.remove<T>(entity)`
- `world.set<T>(entity, newValue)` on the old value
- `world.kill(entity)`
- `World` destruction

Example for cleaning a resource:

```cpp
struct ManagedString {
    const char *value = strdup("");

    static void onRemove(ecs::World &, ecs::Entity, const ManagedString *str) {
        free(const_cast<char *>(str->value));
    }
};
```

## 4. `onSet`

Called after writing the new value.

Real example: `Name::onSet()` validates and synchronizes world name indexing.

```cpp
struct Health {
    int hp = 0;

    static void onSet(ecs::World &, ecs::Entity, const Health *value) {
        if (value->hp < 0) {
            // value is const here; trigger follow-up logic if needed
        }
    }
};
```

## 5. Required components via structural hook

The `HasRequiredComponents` concept is consumed through `required_components()`.

The common approach is inheriting from `Required<A, B, C>`:

```cpp
struct Clicked : Required<Hovered> {};
```

When `Clicked` is added, ECS also adds `Hovered`.

## 6. Actual execution order

On `set<T>(entity, value)`:

1. add component if missing
2. call `onRemove` on current value
3. copy or move the new value
4. call `onSet`

On `add<T>(entity)`:

1. migrate to an archetype containing `T`
2. default-construct if available
3. call `onAdd`
4. add required components

On `remove<T>(entity)`:

1. trigger `On<Remove>` observers attached to the column
2. call `onRemove`
3. migrate to an archetype without `T`

## 7. When to use hooks

Use hooks when behavior is strictly tied to a single component:

- memory allocation or release
- synchronization with a world-level index
- local invariant enforcement

Do not use hooks for:

- multi-component gameplay logic
- frame-timing dependent rules
- behavior that should remain explicit as a system

## 8. Recommended pattern

- `onAdd` to initialize from world context
- `onRemove` to release external resources
- `onSet` to maintain local invariants
- `Required<>` to express structural dependencies

If a component becomes too active, move part of that logic into a system or plugin.