# Arcade

Arcade is a modular game launcher written in C++20.
The executable loads one graphics module and one game module at runtime, both as shared libraries (`.so`).
Because modules communicate through shared interfaces, a game can run on any supported graphics backend.

## Architecture

The project is split into three parts:

- **Core executable**: loads libraries, routes user actions, and manages module switching.
- **Graphics modules**: implement rendering, window/input handling, and timing (`ncurses`, `SDL2`, `SFML`).
- **Game modules**: implement gameplay logic through `Engine` + ECS worlds (for example `Snake`, `Pacman`).

The ECS stack is documented in `docs/` and is shared by all game modules.

## Requirements

- C++20 compiler (`g++` or `clang++`)
- CMake `>= 3.20`
- Graphics dependencies used by your enabled backends (`SFML`, `SDL2`, `SDL2_image`, `SDL2_ttf`, `ncurses`)
- Criterion (optional, only for unit tests)

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Build with tests

```bash
mkdir -p build
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
./unittests
```

### Build options

Important CMake toggles (from `CMakeLists.txt`):

- `BUILD_CORE_PROGRAM`
- `BUILD_PLATFORM_LIBS`
- `BUILD_GAMES`
- `BUILD_TESTS`
- `ENABLE_SANITIZERS`

## Run

Launch with an initial graphics library:

```bash
./arcade ./lib/arcade_ncurses.so
```

Compiled shared libraries are emitted to `build/lib/` by default.

## Documentation

Online documentation: https://akinator31.github.io/Arcade/

Generate Doxygen pages from code and markdown:

```bash
doxygen Doxyfile
```

Then open `html/index.html`.

### Documentation index

- `docs/INDEX.md` - documentation landing page used by Doxygen
- `docs/ECS.md` - ECS overview and reading order
- `docs/ECS_QUICKSTART.md` - ECS quickstart with `Engine` and `ecs::World`
- `docs/ECS_SYSTEMS.md` - ECS systems, phases, observers, and macros (`SYSTEM`, `ITER`, `RUN`, `OBSERVE`)
- `docs/ECS_COMPONENTS.md` - ECS component lifecycle and required components
- `docs/ECS_COMPONENT_HOOKS.md` - ECS hooks: `onAdd`, `onRemove`, `onSet`
- `docs/ECS_PLUGINS.md` - ECS reusable feature bundles
- `docs/ECS_RELATIONS_SINGLETONS_STATE.md` - ECS relations, singleton storage, and typed state
- `docs/DEVELOP_GAME_MODULE.md` - how to implement a new game module
- `docs/DEVELOP_GRAPHICS_LIBRARY.md` - how to implement a new graphics module