# LECS Component Hooks

LECS permet de brancher du comportement autour du cycle de vie des composants.

## 1. Hooks disponibles

Un composant peut définir :

- `static void onAdd(ecs::World &, ecs::Entity)`
- `static void onRemove(ecs::World &, ecs::Entity, const T *)`
- `static void onSet(ecs::World &, ecs::Entity, const T *)`
- `static auto required_components()`

Il peut aussi exposer un constructeur :

- `T()`
- `T(ecs::World &)`

## 2. `onAdd`

Appelé juste après l'ajout du composant.

Exemple réel : `Name::onAdd()` synchronise l'index des noms dans `World`.

```cpp
struct SpawnedAt {
    float time = 0.f;

    static void onAdd(ecs::World &world, ecs::Entity entity) {
        world.get<SpawnedAt>(entity)->time = world.deltaTime;
    }
};
```

## 3. `onRemove`

Appelé avant qu'une valeur soit retirée du monde.

Il s'exécute notamment :

- lors d'un `world.remove<T>(entity)`
- lors d'un `world.set<T>(entity, newValue)` sur l'ancienne valeur
- lors d'un `world.kill(entity)`
- à la destruction du `World`

Exemple important pour nettoyer une ressource :

```cpp
struct ManagedString {
    const char *value = strdup("");

    static void onRemove(ecs::World &, ecs::Entity, const ManagedString *str) {
        free(const_cast<char *>(str->value));
    }
};
```

## 4. `onSet`

Appelé après écriture de la nouvelle valeur.

Exemple réel : `Name::onSet()` valide/synchronise le nom dans l'index du monde.

```cpp
struct Health {
    int hp = 0;

    static void onSet(ecs::World &, ecs::Entity, const Health *value) {
        if (value->hp < 0) {
            // ici tu ne peux pas modifier directement const value,
            // mais tu peux déclencher une autre logique
        }
    }
};
```

## 5. Composants requis via hook structurel

Le concept `HasRequiredComponents` est utilisé via `required_components()`.

Le plus simple est d'hériter de `Required<A, B, C>` :

```cpp
struct Clicked : Required<Hovered> {};
```

À l'ajout de `Clicked`, LECS ajoute aussi `Hovered`.

## 6. Ordre réel d'exécution

Sur `set<T>(entity, value)` :

1. le composant est ajouté s'il n'existe pas
2. `onRemove` est appelé sur la valeur courante
3. la nouvelle valeur est copiée / déplacée
4. `onSet` est appelé

Sur `add<T>(entity)` :

1. migration vers un archetype contenant `T`
2. construction par défaut éventuelle
3. `onAdd`
4. ajout des composants requis

Sur `remove<T>(entity)` :

1. observers `On<Remove>` attachés à la colonne
2. `onRemove`
3. migration vers un archetype sans `T`

## 7. Quand utiliser un hook

Utiliser un hook si le comportement est strictement lié au composant lui-même :

- allocation / libération mémoire
- synchronisation d'un index global
- enforcement d'une contrainte locale

Ne pas l'utiliser pour :

- de la logique de gameplay multi-composants
- des règles dépendantes du frame timing
- des comportements qui devraient être visibles comme systèmes

## 8. Pattern recommandé

- `onAdd` pour initialiser à partir du monde
- `onRemove` pour libérer
- `onSet` pour maintenir une invariant
- `Required<>` pour les dépendances structurelles

Si le composant devient trop "actif", c'est souvent le signe qu'une partie de la logique devrait migrer dans un système ou un plugin.
