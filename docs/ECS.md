# ECS and Engine Overview

This documentation describes the ECS API used by `Engine` and `ecs::World` in this repository.

## Available guides

- [ECS Quickstart](ECS_QUICKSTART.md)
- [ECS Systems](ECS_SYSTEMS.md)
- [ECS Plugins](ECS_PLUGINS.md)
- [ECS Components](ECS_COMPONENTS.md)
- [ECS Component Hooks](ECS_COMPONENT_HOOKS.md)
- [ECS Relations, Singletons, State](ECS_RELATIONS_SINGLETONS_STATE.md)
- [Develop a Game Module](DEVELOP_GAME_MODULE.md)
- [Develop a Graphics Library](DEVELOP_GRAPHICS_LIBRARY.md)

## Recommended reading order

1. Start with [ECS Quickstart](ECS_QUICKSTART.md)
2. Continue with [ECS Systems](ECS_SYSTEMS.md)
3. Read [ECS Components](ECS_COMPONENTS.md) and [ECS Component Hooks](ECS_COMPONENT_HOOKS.md)
4. Read [ECS Plugins](ECS_PLUGINS.md)
5. Finish with [ECS Relations, Singletons, State](ECS_RELATIONS_SINGLETONS_STATE.md)

## Code map

- Engine entry point: `src/engine/Engine.hpp`
- ECS world implementation: `src/engine/ecs/World.hpp`
- Minimal game example: `src/games/example/ExampleGame.cpp`
- ECS-focused tests: `tests/tests_*.cpp`