#include <criterion/criterion.h>

#include "engine/ecs/World.hpp"
#include "engine/plugins/physics/PhysicsPlugin.hpp"

Test(physics, basique) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1;
    auto a = world.create().set(Size(100, 100), Position(0, 0), Velocity(1.0, 0.0)).entity();

    cr_assert_eq(world.get<Position>(a)->x, 0);
    world.progress();
    cr_assert_eq(world.get<Position>(a)->x, 1);
    auto b = world.create().set(Size(100, 100), Position(100, 0), Velocity(0, 0.0)).entity();
    world.progress();
    cr_assert_eq(world.get<Position>(a)->x, 0);
    cr_assert_eq(world.get<Position>(b)->x, 100);

}

Test(physics, advanced) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1;
    auto a = world.create().set(Size(100, 100), Position(0, 0), Velocity(1000.0, 0.0)).entity();

    cr_assert_eq(world.get<Position>(a)->x, 0);
    auto b = world.create().set(Size(100, 100), Position(500, 0), Velocity(0, 0.0)).entity();
    world.progress();
    cr_assert_eq(world.get<Position>(a)->x, 400);
    cr_assert_eq(world.get<Position>(b)->x, 500);

}

Test(move, basique) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1;
    auto a = world.create().set(Size(100, 100), Position(0, 0), Velocity(1.0, 0.0)).entity();

    cr_assert_eq(world.get<Position>(a)->x, 0);
    cr_assert_eq(world.get<Position>(a)->y, 0);
    world.progress();
    cr_assert_eq(world.get<Position>(a)->x, 1);
    cr_assert_eq(world.get<Position>(a)->y, 0);

}