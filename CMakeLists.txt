cmake_minimum_required(VERSION 3.20)
project(arcade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- OPTIONS ---
option(ENABLE_SANITIZERS "Build with sanitizers" OFF)
option(BUILD_PLATFORM_LIBS "Build platform dynamic libraries" ON)
option(BUILD_TESTS "Build unit tests" ON)

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

# --- SOURCE FILES ---
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# --- SFML 3 ---
if (BUILD_PLATFORM_LIBS)
    find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)

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
            SFML::Graphics
    )

    target_compile_options(sfml_api PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
    target_link_options(sfml_api PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
endif ()

# --- MAIN EXECUTABLE ---
add_executable(arcade ${SRC_FILES} src/main.cpp)
target_include_directories(arcade PRIVATE include src)
target_compile_options(arcade PRIVATE ${ARCADE_COMMON_COMPILE_OPTIONS} ${ARCADE_SANITIZER_COMPILE_OPTIONS})
target_link_options(arcade PRIVATE ${ARCADE_SANITIZER_LINK_OPTIONS})
target_link_libraries(arcade PRIVATE ${CMAKE_DL_LIBS})


# --- TESTS ---
if (BUILD_TESTS)
    enable_testing()
    find_library(CRITERION_LIB criterion)

    if (CRITERION_LIB)
        file(GLOB_RECURSE TEST_SRC CONFIGURE_DEPENDS "tests/*.cpp")

        add_executable(unittests ${SRC_FILES} ${TEST_SRC})

        target_compile_options(unittests PRIVATE
                ${ARCADE_COMMON_COMPILE_OPTIONS}
                --coverage
                ${ARCADE_SANITIZER_COMPILE_OPTIONS}
        )

        target_link_options(unittests PRIVATE --coverage ${ARCADE_SANITIZER_LINK_OPTIONS})
        target_include_directories(unittests PRIVATE include src)

        target_link_libraries(unittests PRIVATE ${CRITERION_LIB} ${CMAKE_DL_LIBS})

        add_test(NAME unittests COMMAND unittests)
    else ()
        message(WARNING "Criterion library not found ! Target 'unittests' will not be created.")
    endif ()
endif ()
