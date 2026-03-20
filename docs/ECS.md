# LECS / Engine

Cette documentation couvre l'API ECS utilisée par `Engine` et `ecs::World` dans ce dépôt.

Guides disponibles :

- [LECS Quickstart](/home/suleyman/projects/Arcade/docs/LECS_QUICKSTART.md)
- [LECS Systems](/home/suleyman/projects/Arcade/docs/LECS_SYSTEMS.md)
- [LECS Plugins](/home/suleyman/projects/Arcade/docs/LECS_PLUGINS.md)
- [LECS Components](/home/suleyman/projects/Arcade/docs/LECS_COMPONENTS.md)
- [LECS Component Hooks](/home/suleyman/projects/Arcade/docs/LECS_COMPONENT_HOOKS.md)
- [LECS Relations, Singletons, State](/home/suleyman/projects/Arcade/docs/LECS_RELATIONS_SINGLETONS_STATE.md)

Ordre recommandé :

1. lire le quickstart
2. lire les systems
3. lire components + hooks
4. lire plugins
5. lire relations / singletons / state

Repères dans le code :

- moteur : `src/engine/Engine.hpp`
- monde ECS : `src/engine/ecs/World.hpp`
- exemples : `src/games/example/ExampleGame.cpp`
- tests ECS : `tests/tests_*.cpp`
