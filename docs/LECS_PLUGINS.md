# LECS Plugins

Un plugin sert à encapsuler une feature ECS réutilisable.

## 1. Interface

Un plugin est n'importe quel type qui expose :

```cpp
struct MyPlugin {
    void load(ecs::World &world);
    void unload(ecs::World &world);
};
```

`World` le considère alors comme un plugin valide.

## 2. Chargement

```cpp
world.plugin<MyPlugin>();
world.plugin<StatefulPlugin>(arg1, arg2);
```

Comportement :

- le plugin est instancié une seule fois par type
- les arguments sont transmis au constructeur
- `load(world)` est appelé immédiatement
- `world.hasPlugin<T>()` permet de vérifier sa présence
- `world.getPlugin<T>()` donne accès à l'instance

## 3. Déchargement

```cpp
world.removePlugin<MyPlugin>();
```

Comportement :

- `unload(world)` est appelé
- l'instance est détruite
- `World::~World()` décharge aussi automatiquement les plugins restants

## 4. Exemple minimal

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

## 5. Ce qu'un bon plugin contient

- l'enregistrement de ses composants
- ses systèmes et observers
- ses singletons
- ses états initiaux
- ses relations spécifiques si nécessaire
- son teardown propre dans `unload`

## 6. Exemples du repo

### `RenderPlugin`

Charge :

- `PositionPropagationPlugin`
- composants `Size` et `Color`
- systèmes de rendu rect + sprite

### `SpritePlugin`

Charge :

- composants `Sprite`, `SpriteAnimation`, `SpriteAtlas`
- système `SpriteAnimationSys`

### `PhysicsPlugin`

Charge notamment :

- composants de physique
- systèmes d'intégration et collisions
- des singletons de collision / spatial query

### `DefaultPlugin`

Assemble plusieurs plugins de base pour démarrer rapidement.

## 7. Plugins au niveau `Engine`

Une scène d'`Engine` peut être créée depuis un type de plugin :

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

Dans `Engine::scene<Scene>()`, si `Scene` satisfait `IsPlugin`, le plugin est chargé automatiquement à la création de la scène.

## 8. Recommandations

- un plugin doit être idempotent à l'usage via `world.plugin<T>()`
- centraliser les dépendances d'une feature dans le plugin
- retirer explicitement les systèmes dans `unload`
- nettoyer les singletons dans `unload`
- éviter qu'un plugin dépende d'un ordre externe implicite quand c'est évitable

## 9. Pièges à connaître

- `registerComponent<T>()` est sans effet si le composant est déjà enregistré, donc l'appeler plusieurs fois est toléré
- `world.plugin<T>()` ne recharge pas le plugin s'il est déjà présent
- un plugin qui crée des entités dans `load()` doit aussi réfléchir à leur cycle de vie
