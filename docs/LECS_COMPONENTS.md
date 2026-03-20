# LECS Components

Un composant est une struct de données associée à une entité.

## 1. Définition simple

```cpp
struct Health {
    int hp = 100;
};
```

## 2. Enregistrement

Avant usage, enregistrer le composant dans le monde :

```cpp
world.registerComponent<Health>();
```

Le moteur stocke alors son type, sa construction éventuelle, ses hooks et ses dépendances.

Important :

- si un composant déclare des dépendances via `Required<>`, les types requis doivent aussi être enregistrés
- exemple réel : `Position` requiert `GlobalPosition`, donc il faut enregistrer les deux

## 3. Ajout et accès

```cpp
world.registerComponent<Health>();

const ecs::Entity e = world.entity();
world.add<Health>(e);
world.get<Health>(e)->hp = 75;

bool hasHealth = world.has<Health>(e);
Health *health = world.get<Health>(e);
```

Avec `EntityRef` :

```cpp
world.create()
    .add<Health>()
    .set(Name{"player"});
```

## 4. `add` vs `set`

`add<T>(entity)` :

- ajoute le composant si absent
- construit une valeur par défaut si possible
- déclenche `onAdd`

`set<T>(entity, value)` :

- ajoute le composant si nécessaire
- remplace la valeur courante
- exécute `onRemove` sur l'ancienne valeur avant écriture
- exécute `onSet` après écriture

Utilise `set` quand tu connais déjà la valeur initiale.

## 5. Constructeurs supportés

LECS sait initialiser un composant via :

- `T()`
- `T(ecs::World &world)`

Exemple :

```cpp
struct Health {
    int hp = 100;
};

struct SpawnIndex {
    uint32_t value = 0;

    explicit SpawnIndex(ecs::World &world) : value(world.entity().index) {}
};
```

## 6. Composants utilitaires du repo

Quelques composants de base existent déjà :

- `Name`
- `Position`
- `GlobalPosition`
- `Velocity`
- `Size`
- `Color`
- `Sprite`

Exemple :

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

## 7. Composants requis

Tu peux déclarer qu'un composant dépend d'autres composants.

```cpp
struct Hovered : Required<Position, Size> {};
```

Quand tu fais :

```cpp
world.add<Hovered>(entity);
```

LECS ajoute aussi automatiquement `Position` et `Size`.

Mais ces composants doivent déjà avoir été enregistrés dans le `World`.

## 8. Entités "constructibles"

Ce n'est pas un composant, mais c'est très utile pour créer des presets d'entités.

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

## 9. `Name`

`Name` a un comportement spécial :

- le nom doit être un identifiant valide
- il doit être unique dans le `World`
- si vide, il est remplacé par un nom par défaut du type `entity(index, generation)`
- `findEntityByName()` repose sur ce composant

## 10. Bonnes pratiques

- garder les composants petits et data-only
- éviter d'y mettre de la logique métier complexe
- réserver l'allocation mémoire et les ressources externes aux cas où un hook de nettoyage existe
- utiliser `Required<>` pour exprimer des invariants structurels
