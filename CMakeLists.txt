cmake_minimum_required(VERSION 3.20)
project(arcade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_executable(arcade ${SRC_FILES} src/main.cpp)
target_include_directories(arcade PRIVATE include src)
target_compile_options(arcade PRIVATE -Wall -Wextra -Werror)

# TEST CRITERION

option(BUILD_TESTS "Build unit tests" OFF)

if (BUILD_TESTS)
    enable_testing()
    find_library(CRITERION_LIB criterion)

    if (CRITERION_LIB)
        file(GLOB_RECURSE LIB_SRC "src/*.cpp")
        list(FILTER LIB_SRC EXCLUDE REGEX "src/main\\.cpp$")
        file(GLOB_RECURSE TEST_SRC "tests/*.cpp")

        add_executable(unittests ${LIB_SRC} ${TEST_SRC})

        target_compile_options(unittests PRIVATE -Wall -Wextra --coverage)
        target_link_options(unittests PRIVATE --coverage)
        target_include_directories(unittests PRIVATE include src)
        target_link_libraries(unittests ${CRITERION_LIB})

        add_test(NAME unittests COMMAND unittests)
    else ()
        message(WARNING "Criterion not found, unit tests will not be built.")
    endif ()
endif ()
