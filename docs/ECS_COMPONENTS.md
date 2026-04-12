# ECS Components

A component is a data struct attached to an entity.

## 1. Simple definition

```cpp
struct Health {
    int hp = 100;
};
```

## 2. Registration

Before use, register the component in the world:

```cpp
world.registerComponent<Health>();
```

The engine then tracks its type, constructors, hooks, and required dependencies.

Important:

- if a component declares dependencies with `Required<>`, required types must also be registered
- real example: `Position` requires `GlobalPosition`, so both must be registered

## 3. Add and access

```cpp
world.registerComponent<Health>();

const ecs::Entity e = world.entity();
world.add<Health>(e);
world.get<Health>(e)->hp = 75;

bool hasHealth = world.has<Health>(e);
Health *health = world.get<Health>(e);
```

With `EntityRef`:

```cpp
world.create()
    .add<Health>()
    .set(Name{"player"});
```

## 4. `add` vs `set`

`add<T>(entity)`:

- adds the component when missing
- default-constructs a value when possible
- triggers `onAdd`

`set<T>(entity, value)`:

- adds the component if needed
- replaces current value
- runs `onRemove` on the old value before writing
- runs `onSet` after writing

Use `set` when you already know the initial value.

## 5. Supported constructors

ECS can initialize a component with:

- `T()`
- `T(ecs::World &world)`

Example:

```cpp
struct Health {
    int hp = 100;
};

struct SpawnIndex {
    uint32_t value = 0;

    explicit SpawnIndex(ecs::World &world) : value(world.entity().index) {}
};
```

## 6. Utility components in this repository

Common built-in components:

- `Name`
- `Position`
- `GlobalPosition`
- `Velocity`
- `Size`
- `Color`
- `Sprite`

Example:

```cpp
world.registerComponent<GlobalPosition>();
world.registerComponent<Position>();
world.registerComponent<Velocity>();
world.registerComponent<Size>();
world.registerComponent<Color>();

world.create().set(
    Name{"player"},
    Position{0.f, 0.f},
    Velocity{100.f, 0.f},
    Size{32.f, 32.f},
    Color::WHITE()
);
```

## 7. Required components

You can declare that a component depends on others.

```cpp
struct Hovered : Required<Position, Size> {};
```

When calling:

```cpp
world.add<Hovered>(entity);
```

ECS also adds `Position` and `Size` automatically.

Those component types must already be registered in the same `World`.

## 8. Constructible entity presets

Not a component, but useful for reusable entity recipes.

```cpp
struct PlayerPrefab {
    struct Props {
        float x = 0.f;
        float y = 0.f;
    };

    static void construct(ecs::EntityRef ref, const Props props = {}) {
        ref.set(
            Name{"player"},
            Position{props.x, props.y},
            Velocity{0.f, 0.f}
        );
    }
};

ecs::EntityRef player = world.create<PlayerPrefab>({100.f, 200.f});
```

## 9. `Name` specifics

`Name` has special behavior:

- value must be a valid identifier
- value must be unique in a `World`
- empty values are replaced by a default `entity(index, generation)` name
- `findEntityByName()` depends on this component

## 10. Best practices

- keep components small and data-only
- avoid embedding complex gameplay logic in components
- handle memory/resource cleanup through hooks when needed
- use `Required<>` to express structural invariants