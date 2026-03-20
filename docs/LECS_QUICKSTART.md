# LECS Quickstart

Ce quickstart montre la façon la plus directe d'utiliser `Engine` et `ecs::World` pour faire un jeu.

## 1. Modèle mental

- `Engine` contient une ou plusieurs scènes
- chaque scène est un `ecs::World`
- un `World` contient des entités, composants, systèmes, plugins, états et singletons
- `engine.update(api)` met à jour `world.api`, `world.deltaTime`, puis lance `world.progress()`

## 2. Concepts minimums

- une entité = un identifiant léger
- un composant = une struct de données
- un système = du code qui s'exécute sur les entités qui matchent une query
- un plugin = un paquetage réutilisable qui enregistre composants/systèmes/singletons

## 3. Premier monde

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
    return reinterpret_cast<IGameModule *>(new Engine("MyGame", [](Engine &engine) {
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

## 4. Cycle de frame

`world.progress()` exécute les phases dans cet ordre :

1. `PreUpdate`
2. `Update`
3. `PostUpdate`
4. `PreRender`
5. `Render`

`world.start()` exécute :

1. `PreStartup`
2. `Startup`
3. `PreUpdate`

## 5. API utile immédiatement

Création :

```cpp
ecs::Entity e = world.entity();
ecs::EntityRef ref = world.create();
```

Ajout / écriture :

```cpp
world.add<Position, Velocity>(e);
world.set<Position>(e, {10.f, 20.f});
world.set<Velocity>(e, {1.f, 0.f});
```

Lecture :

```cpp
if (world.has<Position>(e)) {
    auto *pos = world.get<Position>(e);
}
```

Suppression :

```cpp
world.remove<Velocity>(e);
world.kill(e);
```

Recherche par nom :

```cpp
auto found = world.findEntityByName("player");
```

## 6. Style recommandé

- enregistrer explicitement les composants métier
- enregistrer aussi les composants requis par tes composants métier
- utiliser `EntityRef` pour les créations fluides
- mettre les données dans les composants, la logique dans les systèmes
- réserver les singletons et le `state` au contexte global de la scène
- grouper les features réutilisables dans des plugins

## 7. À lire ensuite

- [LECS Systems](/home/suleyman/projects/Arcade/docs/LECS_SYSTEMS.md)
- [LECS Components](/home/suleyman/projects/Arcade/docs/LECS_COMPONENTS.md)
- [LECS Plugins](/home/suleyman/projects/Arcade/docs/LECS_PLUGINS.md)
