cmake_minimum_required(VERSION 3.20)
project(arcade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SANITIZERS "Build with sanitizers" OFF)
option(BUILD_CORE_PROGRAM "Build the core arcade executable" ON)
option(BUILD_PLATFORM_LIBS "Build platform dynamic libraries" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_GAMES "Build game dynamic libraries" ON)

include(FetchContent)
FetchContent_Declare(
        arcade_shared_iface
        GIT_REPOSITORY https://github.com/arcade-tls/Interfaces.git
        GIT_TAG main
)
FetchContent_MakeAvailable(arcade_shared_iface)

add_library(arcade_shared_iface INTERFACE)
target_include_directories(arcade_shared_iface INTERFACE
        ${arcade_shared_iface_SOURCE_DIR}
)

set(ARCADE_COMMON_COMPILE_OPTIONS -Wall -Wextra -Werror)
set(ARCADE_SANITIZER_COMPILE_OPTIONS "")
set(ARCADE_SANITIZER_LINK_OPTIONS "")

if (ENABLE_SANITIZERS)

endif ()

file(GLOB_RECURSE ECS_SRC CONFIGURE_DEPENDS "src/engine/*.cpp")
file(GLOB_RECURSE TEST_SRC CONFIGURE_DEPENDS "tests/*.cpp")

add_library(arcade_shared INTERFACE)
target_include_directories(arcade_shared INTERFACE src)

file(GLOB_RECURSE ALL_ENGINE_HEADERS CONFIGURE_DEPENDS "src/engine/*.hpp")
add_library(arcade_engine_headers INTERFACE)
target_include_directories(arcade_engine_headers INTERFACE src src/engine)
target_sources(arcade_engine_headers INTERFACE ${ALL_ENGINE_HEADERS})
target_link_libraries(arcade_engine_headers INTERFACE arcade_shared_iface)

if (BUILD_GAMES OR BUILD_TESTS)
    add_library(arcade_ecs OBJECT ${ECS_SRC})
    set_target_properties(arcade_ecs PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(arcade_ecs PUBLIC src)
    target_link_libraries(arcade_ecs PUBLIC arcade_shared_iface arcade_engine_headers)
    target_compile_options(arcade_ecs PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
endif ()

if (BUILD_PLATFORM_LIBS)
    find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)

    add_library(sfml_api SHARED
            src/graphics/sfml/SfmlApi.cpp
            src/graphics/sfml/SfmlApi.hpp
    )
    target_include_directories(sfml_api PRIVATE src/graphics/sfml)
    target_link_libraries(sfml_api PRIVATE arcade_shared arcade_shared_iface SFML::Graphics)
    target_compile_options(sfml_api PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(sfml_api PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
    set_target_properties(sfml_api PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            OUTPUT_NAME arcade_sfml
            PREFIX ""
    )

    add_library(sdl_api SHARED
            src/graphics/sdl/SdlApi.cpp
            src/graphics/sdl/SdlApi.hpp
    )
    target_include_directories(sdl_api PRIVATE src/graphics/sdl)
    target_link_libraries(sdl_api PRIVATE
            arcade_shared
            arcade_shared_iface
            SDL2::SDL2
            SDL2::SDL2main
            SDL2_image::SDL2_image
            SDL2_ttf::SDL2_ttf
    )
    target_compile_options(sdl_api PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(sdl_api PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
    set_target_properties(sdl_api PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            OUTPUT_NAME arcade_sdl
            PREFIX ""
    )
endif ()

if (BUILD_CORE_PROGRAM)
    add_executable(arcade src/main.cpp)
    target_include_directories(arcade PRIVATE src/core)
    target_link_libraries(arcade PRIVATE ${CMAKE_DL_LIBS} arcade_shared_iface arcade_engine_headers)
    target_compile_options(arcade PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(arcade PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
endif ()

if (BUILD_GAMES)
    add_library(game_menu SHARED
            src/games/menu/GameMenu.cpp
            src/games/menu/GameMenu.hpp
    )
    target_sources(game_menu PRIVATE $<TARGET_OBJECTS:arcade_ecs>)
    target_include_directories(game_menu PRIVATE src src/games/menu)
    target_link_libraries(game_menu PRIVATE arcade_shared arcade_shared_iface arcade_engine_headers)
    target_compile_options(game_menu PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(game_menu PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
    set_target_properties(game_menu PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            OUTPUT_NAME arcade_game_menu
            PREFIX ""
    )

    add_library(pacman_game SHARED
            src/games/pacman/Pacman.cpp
    )
    target_sources(pacman_game PRIVATE $<TARGET_OBJECTS:arcade_ecs>)
    target_include_directories(pacman_game PRIVATE src src/games/pacman)
    target_link_libraries(pacman_game PRIVATE arcade_shared arcade_shared_iface)
    target_compile_options(pacman_game PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(pacman_game PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
    set_target_properties(pacman_game PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            OUTPUT_NAME arcade_game_pacman
            PREFIX ""
    )

    add_library(snake_game SHARED
            src/games/snake/Snake.cpp
            src/games/snake/SnakeGameplayPlugin.hpp
    )
    target_sources(snake_game PRIVATE $<TARGET_OBJECTS:arcade_ecs>)
    target_include_directories(snake_game PRIVATE src src/games/snake)
    target_link_libraries(snake_game PRIVATE arcade_shared arcade_shared_iface)
    target_compile_options(snake_game PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(snake_game PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
    set_target_properties(snake_game PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            OUTPUT_NAME arcade_game_snake
            PREFIX ""
    )
endif ()

if (BUILD_TESTS)
    enable_testing()
    find_library(CRITERION_LIB criterion)

    if (CRITERION_LIB)
        add_executable(unittests ${TEST_SRC})
        target_sources(unittests PRIVATE $<TARGET_OBJECTS:arcade_ecs>)
        target_include_directories(unittests PRIVATE src tests)
        target_link_libraries(unittests PRIVATE arcade_shared arcade_shared_iface ${CRITERION_LIB} ${CMAKE_DL_LIBS} arcade_engine_headers)
        target_compile_options(unittests PRIVATE
                ${ARCADE_COMMON_COMPILE_OPTIONS}
                --coverage
                ${ARCADE_SANITIZER_COMPILE_OPTIONS}
        )
        target_link_options(unittests PRIVATE --coverage ${ARCADE_SANITIZER_LINK_OPTIONS})
        add_test(NAME unittests COMMAND unittests)
    else ()
        message(WARNING "Criterion library not found ! Target 'unittests' will not be created.")
    endif ()
endif ()
