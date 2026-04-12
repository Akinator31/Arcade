# Develop a Graphics Library

This guide explains how to implement a new graphics backend shared library (`.so`) for Arcade.

## 1. Understand the contract

A graphics library must implement `IDisplayModule` and expose:

```cpp
extern "C" IDisplayModule *load();
extern "C" void unload(IDisplayModule *api);
```

The core resolves these symbols through `GraphicsApiLoader` (`src/core/dynamic/GraphicsLoader.hpp`).

## 2. Required runtime flow

The core calls graphics modules in this order (see `src/main.cpp`):

1. `load()`
2. `api->init()`
3. `api->loadResources(game->getResources())`
4. per frame: `beginFrame()`, game update, `endFrame()`
5. on shutdown/switch: `api->shutdown()`, `unload(api)`

Your implementation must be safe for backend switching at runtime.

## 3. Implement all `IDisplayModule` methods

Use existing backends as reference:

- `src/graphics/sfml/SfmlApi.hpp`
- `src/graphics/sdl/SdlApi.hpp`
- `src/graphics/ncurses/NcursesApi.hpp`

Core methods to implement:

- lifecycle: `init`, `shutdown`, `isWindowOpen`
- frame flow: `beginFrame`, `endFrame`
- window: `setWindowSize`, `getWindowSize`
- input: `isKeyPressed`, `getMousePosition`, `isMouseButtonPressed`, `wasMouseButtonReleased`
- draw API: `drawRect`, `drawRectOutline`, `drawSprite`, `drawText`
- resources and clear color: `loadResources`, `setClearColor`

## 4. Export symbols

In your backend `.cpp`:

```cpp
extern "C" IDisplayModule *load() {
    return new MyGraphicsApi();
}

extern "C" void unload(IDisplayModule *api) {
    delete api;
}
```

Use C linkage (`extern "C"`) to avoid C++ name mangling.

## 5. Add the backend to CMake

In `CMakeLists.txt`, inside `if (BUILD_PLATFORM_LIBS)`, add a new shared target.

Pattern:

```cmake
add_library(my_api SHARED
    src/graphics/myapi/MyApi.cpp
    src/graphics/myapi/MyApi.hpp
)
target_include_directories(my_api PRIVATE src/graphics/myapi)
target_link_libraries(my_api PRIVATE arcade_shared arcade_shared_iface)
set_target_properties(my_api PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME arcade_myapi
    PREFIX ""
)
```

Add external dependencies with `find_package(...)` and `target_link_libraries(...)` as needed.

## 6. Input mapping guidance

Map backend-specific events to Arcade types consistently.

Recommendations:

- keep key translation centralized in one helper function
- update pressed/released states inside `beginFrame()` polling
- reset one-frame signals (like mouse release) every frame

## 7. Resource handling guidance

`loadResources` receives a vector describing textures/fonts requested by the current game.

Recommendations:

- preserve resource order so `ResourceIndex` stays stable
- keep backend-specific handles in a dedicated container
- gracefully handle missing files (log/fallback instead of crashing when possible)

## 8. Validation checklist

- symbols `load` and `unload` are exported
- backend library is generated in `build/lib/`
- backend can be used as startup argument to `./arcade`
- switching graphics backend at runtime works repeatedly
- no resource leaks after repeated load/switch/unload cycles