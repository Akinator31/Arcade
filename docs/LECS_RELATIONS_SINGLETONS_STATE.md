# LECS Relations, Singletons, State

Ce guide couvre les mécaniques globales et les liens entre entités.

## 1. Relations

Une relation relie une entité source à une entité cible.

Déclaration :

```cpp
struct ChildOf {};
world.relation<ChildOf>();
```

Cela enregistre automatiquement deux composants internes :

- `RelationTarget<ChildOf>` sur l'enfant
- `RelationSource<ChildOf>` sur le parent

## 2. Créer et retirer une relation

```cpp
world.relate<ChildOf>(child, parent);
world.unrelate<ChildOf>(child);
```

Helpers :

```cpp
world.has_target<ChildOf>(child, parent);
world.has_source<ChildOf>(parent, child);
```

## 3. Itérer les relations

Direct seulement :

```cpp
for (auto [parent, entity] : world.iterRelated<ChildOf>(root)) {
}
```

Récursif :

```cpp
for (auto [parent, entity] : world.iterRelated<ChildOf, true>(root)) {
}
```

Le mode récursif parcourt toute la hiérarchie descendante.

## 4. `Hierarchy`

Le moteur définit déjà :

```cpp
struct Hierarchy : ecs::DespawnRelated {};
using Parent = RelationTarget<Hierarchy>;
using Children = RelationSource<Hierarchy>;
```

`EntityRef::child()` crée justement un enfant avec cette relation :

```cpp
ecs::EntityRef child = parentRef.child();
```

## 5. Despawn et relations

Par défaut :

- si la source meurt, elle est retirée de la liste du parent
- si la cible meurt, les sources sont simplement "détachées"

Si la relation hérite de `ecs::DespawnRelated` :

```cpp
struct OwnedBy : ecs::DespawnRelated {};
```

alors détruire la cible détruit aussi toutes les entités liées.

## 6. Singletons

Un singleton est une donnée globale stockée une seule fois par `World`.

API :

```cpp
world.singleton_init<MySingleton>();
world.singleton_init(new MySingleton(...));
MySingleton *value = world.singleton_get<MySingleton>();
bool exists = world.singleton_has<MySingleton>();
world.singleton_remove<MySingleton>();
```

Usage typique :

- spatial index
- cache global
- état runtime partagé par plusieurs systèmes

Exemple :

```cpp
struct CameraSettings {
    float zoom = 1.f;
};

world.singleton_init<CameraSettings>();
world.singleton_get<CameraSettings>()->zoom = 2.f;
```

Dans le repo, la physique utilise des singletons pour `SpatialQuery` et l'état des collisions.

## 7. `state`

`state` est plus léger qu'un singleton. C'est un registre typé de valeurs enum / entières indexées par type.

API :

```cpp
world.state(GameState::Playing);
GameState state = world.getState<GameState>();
```

Pattern idéal :

- état de jeu
- mode d'input
- étape UI
- machine à états simple

Exemple réel du repo :

```cpp
enum class MouseButtonLeftState {
    NONE,
    RELEASED,
    PRESSED,
};

world.state(MouseButtonLeftState::NONE);
```

Puis un système peut filtrer avec :

```cpp
InState<MouseButtonLeftState::RELEASED>
```

## 8. `state` vs singleton

Utilise `state` si :

- tu stockes une petite valeur triviale
- tu veux une condition de système simple
- tu manipules une enum ou un code d'état

Utilise un singleton si :

- tu as plusieurs champs
- tu stockes une structure complexe
- tu veux un objet partagé mutable

## 9. Événements d'entité

Même si ce n'est ni une relation ni un singleton, c'est souvent utilisé avec eux.

```cpp
struct Damaged {
    int value;
};

world.listen<Damaged>(entity, [](ecs::World &world, ecs::Entity e, const Damaged evt) {
});

world.emit(entity, Damaged{10});
```

Caractéristiques :

- un handler par paire `(entity, event type)`
- un nouvel appel à `listen` remplace l'ancien handler pour cette paire
- pratique pour clics UI, collisions, dégâts, triggers

## 10. Recommandations d'architecture

- relations pour les graphes d'entités
- `Hierarchy` pour la parenté scène / UI
- `state` pour les gates de systèmes
- singletons pour les services runtime d'une scène
- événements d'entité pour les interactions ciblées
