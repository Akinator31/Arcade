# ECS Relations, Singletons, State

This guide covers global world mechanics and links between entities.

## 1. Relations

A relation links a source entity to a target entity.

Declaration:

```cpp
struct ChildOf {};
world.relation<ChildOf>();
```

This automatically registers two internal components:

- `RelationTarget<ChildOf>` on the child
- `RelationSource<ChildOf>` on the parent

## 2. Create and remove a relation

```cpp
world.relate<ChildOf>(child, parent);
world.unrelate<ChildOf>(child);
```

Helpers:

```cpp
world.has_target<ChildOf>(child, parent);
world.has_source<ChildOf>(parent, child);
```

## 3. Iterate relations

Direct children only:

```cpp
for (auto [parent, entity] : world.iterRelated<ChildOf>(root)) {
}
```

Recursive traversal:

```cpp
for (auto [parent, entity] : world.iterRelated<ChildOf, true>(root)) {
}
```

Recursive mode walks the full descendant hierarchy.

## 4. `Hierarchy`

The engine already defines:

```cpp
struct Hierarchy : ecs::DespawnRelated {};
using Parent = RelationTarget<Hierarchy>;
using Children = RelationSource<Hierarchy>;
```

`EntityRef::child()` creates a child with that relation:

```cpp
ecs::EntityRef child = parentRef.child();
```

## 5. Despawn and relations

Default behavior:

- if the source dies, it is removed from the parent source list
- if the target dies, sources are detached

If the relation inherits `ecs::DespawnRelated`:

```cpp
struct OwnedBy : ecs::DespawnRelated {};
```

then destroying the target also destroys all linked source entities.

## 6. Singletons

A singleton is global world data stored once per `World`.

API:

```cpp
world.singleton_init<MySingleton>();
world.singleton_init(new MySingleton(...));
MySingleton *value = world.singleton_get<MySingleton>();
bool exists = world.singleton_has<MySingleton>();
world.singleton_remove<MySingleton>();
```

Typical use:

- spatial index
- global cache
- runtime data shared by multiple systems

Example:

```cpp
struct CameraSettings {
    float zoom = 1.f;
};

world.singleton_init<CameraSettings>();
world.singleton_get<CameraSettings>()->zoom = 2.f;
```

In this repository, physics uses singletons for `SpatialQuery` and collision state.

## 7. Typed `state`

`state` is lighter than singletons. It stores enum/integer values keyed by type.

API:

```cpp
world.state(GameState::Playing);
GameState state = world.getState<GameState>();
```

Good fit for:

- game mode
- input mode
- UI step
- simple state machines

Real repository example:

```cpp
enum class MouseButtonLeftState {
    NONE,
    RELEASED,
    PRESSED,
};

world.state(MouseButtonLeftState::NONE);
```

Then systems can filter with:

```cpp
InState<MouseButtonLeftState::RELEASED>
```

## 8. `state` vs singleton

Use `state` when:

- data is a small trivial value
- you need simple system gating
- value is an enum or status code

Use singleton when:

- you need multiple fields
- you store a complex structure
- you need a shared mutable object

## 9. Entity events

Not a relation/singleton feature, but commonly used with both.

```cpp
struct Damaged {
    int value;
};

world.listen<Damaged>(entity, [](ecs::World &world, ecs::Entity e, const Damaged evt) {
});

world.emit(entity, Damaged{10});
```

Properties:

- one handler per `(entity, event type)` pair
- a new `listen` call replaces the existing handler for that pair
- useful for UI clicks, collisions, damage, and triggers

## 10. Architecture recommendations

- use relations for entity graphs
- use `Hierarchy` for scene and UI parent-child structures
- use `state` for system gates
- use singletons for world runtime services
- use entity events for targeted interactions