cmake_minimum_required(VERSION 3.20)
project(arcade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SANITIZERS "Build with sanitizers" OFF)
option(BUILD_CORE_PROGRAM "Build the core arcade executable" ON)
option(BUILD_PLATFORM_LIBS "Build platform dynamic libraries" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_GAMES "Build game dynamic libraries" ON)

set(ARCADE_COMMON_COMPILE_OPTIONS -Wall -Wextra -Werror)
set(ARCADE_SANITIZER_COMPILE_OPTIONS "")
set(ARCADE_SANITIZER_LINK_OPTIONS "")

if (ENABLE_SANITIZERS)
    list(APPEND ARCADE_SANITIZER_COMPILE_OPTIONS
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -g3
            -O0
    )
    list(APPEND ARCADE_SANITIZER_LINK_OPTIONS -fsanitize=address,undefined)
endif ()

file(GLOB_RECURSE ECS_SRC CONFIGURE_DEPENDS "engine/*.cpp")
file(GLOB_RECURSE TEST_SRC CONFIGURE_DEPENDS "tests/*.cpp")

add_library(arcade_shared INTERFACE)
target_include_directories(arcade_shared INTERFACE shared .)

if (BUILD_GAMES OR BUILD_TESTS)
    add_library(arcade_ecs OBJECT ${ECS_SRC})
    set_target_properties(arcade_ecs PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(arcade_ecs PUBLIC shared .)
    target_compile_options(arcade_ecs PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
endif ()

if (BUILD_PLATFORM_LIBS)
    find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)

    add_library(sfml_api SHARED
            graphics/sfml/SfmlApi.cpp
            graphics/sfml/SfmlApi.hpp
    )

    target_include_directories(sfml_api PRIVATE shared graphics/sfml)
    target_link_libraries(sfml_api PRIVATE arcade_shared)

    target_link_libraries(sfml_api
            PRIVATE
            SFML::Graphics
    )

    target_compile_options(sfml_api PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(sfml_api PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
endif ()

if (BUILD_CORE_PROGRAM)
    add_executable(arcade core/main.cpp)
    target_include_directories(arcade PRIVATE shared core)
    target_compile_options(arcade PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(arcade PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
    target_link_libraries(arcade PRIVATE ${CMAKE_DL_LIBS})
endif ()

if (BUILD_GAMES)
    add_library(game_example SHARED
            games/example/ExampleGame.cpp
            games/example/ExampleGame.hpp
    )
    target_sources(game_example PRIVATE $<TARGET_OBJECTS:arcade_ecs>)
    target_include_directories(game_example PRIVATE shared . games/example)
    target_link_libraries(game_example PRIVATE arcade_shared)
    target_compile_options(game_example PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(game_example PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
endif ()

if (BUILD_TESTS)
    enable_testing()
    find_library(CRITERION_LIB criterion)

    if (CRITERION_LIB)
        add_executable(unittests ${TEST_SRC})
        target_sources(unittests PRIVATE $<TARGET_OBJECTS:arcade_ecs>)

        target_compile_options(unittests PRIVATE
                ${ARCADE_COMMON_COMPILE_OPTIONS}
                --coverage
                ${ARCADE_SANITIZER_COMPILE_OPTIONS}
        )

        target_link_options(unittests PRIVATE --coverage ${ARCADE_SANITIZER_LINK_OPTIONS})
        target_include_directories(unittests PRIVATE shared . tests)
        target_link_libraries(unittests PRIVATE arcade_shared ${CRITERION_LIB} ${CMAKE_DL_LIBS})

        add_test(NAME unittests COMMAND unittests)
    else ()
        message(WARNING "Criterion library not found ! Target 'unittests' will not be created.")
    endif ()
endif ()
