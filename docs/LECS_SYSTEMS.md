# LECS Systems

Ce guide couvre les systèmes "batch", les systèmes à état, les observers, les phases et les conditions.

## 1. Formes de systèmes

LECS supporte trois grandes formes.

### Système batch avec `iter`

Le plus courant. Il tourne sur toutes les tables qui matchent `With<>` et `Without<>`.

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

### Système avec état interne et `run`

Quand le système doit mémoriser quelque chose entre deux frames.

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

Le moteur garde l'instance vivante tant que le système est enregistré.

### Système mixte

Un système peut avoir `iter` et `run`.

- `iter` parcourt les entités matchées
- `run` exécute une logique globale

## 2. Déclaration rapide avec les macros

Macros disponibles :

- `SYSTEM(Name, ...)`
- `ITER(view)`
- `RUN(world)`
- `OBSERVE(table, row)`

Exemple :

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

## 3. Sélection des entités

### `With`

Toutes les entités doivent avoir les composants listés.

```cpp
struct RenderRects : With<GlobalPosition, Size, Color> { ... };
```

### `Without`

Exclut les entités portant certains composants.

```cpp
struct MoveOnlyFreeBodies : With<Position, Velocity>, Without<RigidBody> { ... };
```

### Accès dans `ArchetypeView`

- `view.count()`
- `view.entity(i)`
- `view.entities()`
- `view.column<T>()`
- `view.optional<T>()`
- `view.has<T>()`
- `view.world`

Important :

- `column<T>()` suppose que la query exige `T`
- `optional<T>()` retourne `nullptr` si la table n'a pas `T`
- un système itère par archetype, pas entité par entité au niveau haut

## 4. Phases

Par défaut, un système est ajouté dans `Update`.

Pour choisir une phase :

```cpp
struct MySys : With<Position>, On<PreRender> {
    static void iter(ArchetypeView &view) { }
};
```

Phases disponibles :

- `PreStartup`
- `Startup`
- `PostStartup`
- `PreUpdate`
- `Update`
- `PostUpdate`
- `PreRender`
- `Render`

Dans `World::progress()`, seules ces phases sont jouées :

- `PreUpdate`
- `Update`
- `PostUpdate`
- `PreRender`
- `Render`

`PostStartup` existe dans les types, mais n'est pas exécutée par `start()` ni `progress()` dans l'état actuel du code.

## 5. Conditions

Un système peut avoir une condition statique.

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

Conditions composées :

```cpp
struct Sys : With<Position>, Conditions<A, B> {
    static void iter(ArchetypeView &view) { }
};
```

Helpers fournis :

- `InState<VALUE>` pour exécuter un système seulement si `world.getState<decltype(VALUE)>() == VALUE`
- `Interval<MS>` pour exécuter périodiquement selon `world.deltaTime`

Exemple :

```cpp
SYSTEM(ClickSys, With<HoveredComponent>, On<PostUpdate>, InState<MouseButtonLeftState::RELEASED>) {
    ITER(view) {
        for (uint32_t i = 0; i < view.count(); i += 1) {
            view.world.emit(view.entity(i), ClickedEvent{});
        }
    }
};
```

## 6. Enregistrement et suppression

```cpp
const SystemId id = world.system<MoveSystem>();
world.runSystem(id);
world.remove<MoveSystem>();
```

Notes :

- `world.system<T>()` construit l'instance si `T()` ou `T(world)` existe
- `world.remove<T>()` retire toutes les occurrences de ce type de système
- `runSystem(id)` exécute un seul système, sans vérifier la `condition` et sans `flush()` automatique
- `progress()` exécute tous les systèmes des phases runtime

## 7. Observers

Un observer n'utilise pas `iter`, mais `observe`.

### Observer sur ajout

```cpp
struct OnAddPlayer : With<Player>, On<Add> {
    static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
        // déclenché quand une entité entre dans une table avec Player
    }
};
```

### Observer sur suppression de composant

```cpp
struct OnRemovePlayer : With<Player>, On<Remove> {
    static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
    }
};
```

### Observer sur despawn

```cpp
struct OnDespawnPlayer : With<Player>, On<Despawn> {
    static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
    }
};
```

Comportement utile à connaître :

- si l'observer est enregistré après la création d'une table, il est quand même attaché aux tables déjà existantes
- `On<Remove>` s'accroche aux callbacks `onRemove` des colonnes de composants requises
- `On<Despawn>` se déclenche au `world.kill(entity)`

## 8. Commandes différées

Quand un système modifie la structure du monde pendant une itération, mieux vaut différer.

```cpp
view.world.command([entity = view.entity(i)](ecs::World &world) {
    world.add<HoveredComponent>(entity);
});
```

Les commandes sont flushées après chaque système dans `runAll()`.

À utiliser pour :

- ajouter ou retirer des composants pendant une itération
- créer ou tuer des entités depuis un système batch
- émettre des changements structurels sans casser le parcours en cours

## 9. Conseils pratiques

- préférer `iter` pour la logique data-oriented
- préférer `run` pour l'orchestration globale
- éviter les effets de bord structurels directs au milieu d'un `iter`
- utiliser `Without<>` pour séparer des catégories d'entités sans dupliquer les systèmes
- utiliser `EntityRef` comme système seulement si le système représente une entité persistante

## 10. Pattern réel du repo

Le jeu d'exemple utilise un système-objet qui hérite de `ecs::EntityRef` :

```cpp
SYSTEM(PlayerSys, ecs::EntityRef, On<Update>) {
    explicit PlayerSys(ecs::World &world) : EntityRef(
        world.create().set(Position{0, 1500}, Size{100, 100}, Velocity{10, 0}, Color::WHITE())
    ) {}

    RUN(world) {
        if (this->world.api->isKeyPressed(KeyboardCode::Space) && get<Velocity>()->y == 0) {
            get<Velocity>()->y += 100;
        }
    }
};
```

Ce pattern marche bien pour un joueur unique, une caméra, un curseur ou un manager incarné par une entité.
