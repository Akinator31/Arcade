cmake_minimum_required(VERSION 3.20)
project(arcade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SANITIZERS "Build with sanitizers" OFF)

set(ARCADE_COMMON_COMPILE_OPTIONS -Wall -Wextra -Werror)
set(ARCADE_SANITIZER_COMPILE_OPTIONS)
set(ARCADE_SANITIZER_LINK_OPTIONS)

if (ENABLE_SANITIZERS)
    list(APPEND ARCADE_SANITIZER_COMPILE_OPTIONS
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -g3
            -O0
    )
    list(APPEND ARCADE_SANITIZER_LINK_OPTIONS -fsanitize=address,undefined)
endif ()


file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system)

add_library(sfml_api SHARED
        platforms/SfmlApi.cpp
        platforms/SfmlApi.hpp
)
target_include_directories(sfml_api
        PUBLIC
        src
        platforms
)
target_link_libraries(sfml_api
        PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
)
target_compile_options(sfml_api PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
target_link_options(sfml_api PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})

add_executable(arcade ${SRC_FILES} src/main.cpp
        src/engine/ecs/Query.hpp
        src/engine/datastructures/EcsString.hpp
        src/engine/ecs/internal/EventRegistry.hpp
        src/engine/ecs/Component.hpp
        src/engine/ecs/Timer.hpp
        src/engine/Graphics.hpp
        src/engine/ecs/State.hpp
        src/engine/reflection/rayflect.hpp
        src/engine/plugins/CliPlugin.hpp
        src/engine/parsing/Scanner.cpp
        src/engine/plugins/NormalMode.cpp
        src/engine/plugins/EntityMode.cpp
        src/engine/ecs/Singleton.hpp
        src/engine/plugins/CliPlugin.cpp
        src/engine/dynamic/DynamicLoader.hpp
        src/engine/dynamic/Loader.hpp
        src/engine/plugins/render/RenderPlugin.hpp
        src/engine/IEngine.hpp
)
target_include_directories(arcade PRIVATE include src)
target_compile_options(arcade PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
target_link_options(arcade PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
target_link_libraries(arcade PRIVATE dl)


# TEST CRITERION

option(BUILD_TESTS "Build unit tests" OFF)

if (BUILD_TESTS)
    enable_testing()
    find_library(CRITERION_LIB criterion)

    if (CRITERION_LIB)
        file(GLOB_RECURSE LIB_SRC CONFIGURE_DEPENDS "src/*.cpp")
        list(FILTER LIB_SRC EXCLUDE REGEX "src/main\\.cpp$")
        file(GLOB_RECURSE TEST_SRC CONFIGURE_DEPENDS "tests/*.cpp")

        add_executable(unittests ${LIB_SRC} ${TEST_SRC}
                tests/tests_ecs.cpp
                tests/tests_system.cpp
                tests/tests_batch.cpp
                tests/tests_plugin.cpp
                tests/tests_observer.cpp
                tests/tests_entity_event.cpp
                tests/tests_component.cpp
                tests/tests_relation.cpp
                tests/tests_rayflect.cpp
                tests/tests_cli.cpp
                tests/tests_json.cpp
                tests/tests_transform_propagation.cpp)

        target_compile_options(unittests PRIVATE
                ${ARCADE_COMMON_COMPILE_OPTIONS}
                --coverage
                ${ARCADE_SANITIZER_COMPILE_OPTIONS}
        )
        target_link_options(unittests PRIVATE --coverage ${ARCADE_SANITIZER_LINK_OPTIONS})
        target_include_directories(unittests PRIVATE include src)
        target_link_libraries(unittests ${CRITERION_LIB})

        add_test(NAME unittests COMMAND unittests)
    else ()
        message(WARNING "Criterion not found, unit tests will not be built.")
    endif ()
endif ()
