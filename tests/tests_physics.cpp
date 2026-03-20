#include <criterion/criterion.h>

#include "engine/ecs/World.hpp"
#include "engine/plugins/control/ControlPlugin.hpp"
#include "engine/plugins/physics/PhysicsPlugin.hpp"

namespace {
    int collision_start_count = 0;
    int collision_end_count = 0;
    ecs::Entity collision_last_target{0, 0};

    void onCollisionStart(ecs::World &, ecs::Entity, const CollisionStart evt) {
        collision_start_count += 1;
        collision_last_target = evt.target;
    }

    void onCollisionEnd(ecs::World &, ecs::Entity, const CollisionEnd evt) {
        collision_end_count += 1;
        collision_last_target = evt.target;
    }
}

Test(physics, integrate_velocity_without_rigidbody) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1.f;
    const auto entity = world.create().set(Size{10.f, 10.f}, Position(0.f, 0.f), Velocity(3.f, 2.f)).
            entity();

    world.progress();

    cr_assert_float_eq(world.get<Position>(entity)->x, 3.f, 0.001f);
    cr_assert_float_eq(world.get<Position>(entity)->y, 2.f, 0.001f);
}

Test(physics, falling_entity_stops_on_floor_and_keeps_horizontal_speed) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1.f;
    world.create().set(RigidBody::RIGID, Size{100.f, 20.f}, Position(0.f, 100.f), Velocity(0.f, 0.f));
    const auto entity = world.create().set(RigidBody::RIGID, Size{10.f, 10.f}, Position(10.f, 0.f), Velocity(5.f, 120.f)).
            entity();

    world.progress();

    cr_assert_float_eq(world.get<Position>(entity)->x, 15.f, 0.001f);
    cr_assert_float_eq(world.get<Position>(entity)->y, 90.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->x, 5.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->y, 0.f, 0.001f);
}

Test(physics, gravity_driven_rigidbody_lands_and_stays_grounded_across_frames) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 0.1f;
    world.create().set(RigidBody::RIGID, Size{100.f, 20.f}, Position(-20.f, 100.f), Velocity(0.f, 0.f));
    const auto entity = world.create().set(
        RigidBody::RIGID,
        Size{10.f, 10.f},
        Position(0.f, 0.f),
        Velocity(3.f, 0.f),
        Gravity{100.f}
    ).entity();

    for (int i = 0; i < 20; i += 1) {
        world.progress();
    }

    cr_assert_float_eq(world.get<Position>(entity)->x, 6.f, 0.001f);
    cr_assert_float_eq(world.get<Position>(entity)->y, 90.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->x, 3.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->y, 0.f, 0.001f);

    world.progress();

    cr_assert_float_eq(world.get<Position>(entity)->x, 6.3f, 0.001f);
    cr_assert_float_eq(world.get<Position>(entity)->y, 90.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->y, 0.f, 0.001f);
}

Test(physics, entity_does_not_pass_through_wall) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1.f;
    world.create().set(RigidBody::RIGID, Size{20.f, 100.f}, Position(100.f, 0.f), Velocity(0.f, 0.f));
    const auto entity = world.create().set(RigidBody::RIGID, Size{10.f, 10.f}, Position(0.f, 10.f), Velocity(150.f, 0.f)).
            entity();

    world.progress();

    cr_assert_float_eq(world.get<Position>(entity)->x, 90.f, 0.001f);
    cr_assert_float_eq(world.get<Position>(entity)->y, 10.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->x, 0.f, 0.001f);
}

Test(physics, emit_collision_start_and_end_events) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1.f;
    const ecs::Entity floor = world.create().set(RigidBody::RIGID, Size{100.f, 20.f}, Position(0.f, 100.f), Velocity(0.f, 0.f)).
            entity();
    const ecs::Entity entity = world.create().set(
        EmitCollisionEvent{},
        RigidBody::RIGID,
        Size{10.f, 10.f},
        Position(10.f, 0.f),
        Velocity(0.f, 120.f)
    ).entity();

    collision_start_count = 0;
    collision_end_count = 0;
    collision_last_target = {0, 0};

    world.listen<CollisionStart>(entity, onCollisionStart);
    world.listen<CollisionEnd>(entity, onCollisionEnd);

    world.progress();
    cr_assert_eq(collision_start_count, 1);
    cr_assert_eq(collision_end_count, 0);
    cr_assert(collision_last_target == floor);

    world.progress();
    cr_assert_eq(collision_start_count, 1);
    cr_assert_eq(collision_end_count, 0);

    world.set<Position>(entity, {10.f, 0.f});
    world.set<Velocity>(entity, {0.f, 0.f});
    world.progress();
    cr_assert_eq(collision_start_count, 1);
    cr_assert_eq(collision_end_count, 1);
    cr_assert(collision_last_target == floor);
}

Test(physics, rigidbody_with_emit_events_detects_sensor_without_being_blocked) {
    ecs::World world;
    world.plugin<PhysicsPlugin>();

    world.deltaTime = 1.f;
    const ecs::Entity sensor = world.create().set(
        RigidBody::SENSOR,
        Size{20.f, 100.f},
        Position(140.f, 0.f),
        Velocity(0.f, 0.f)
    ).entity();
    const ecs::Entity entity = world.create().set(
        EmitCollisionEvent{},
        RigidBody::RIGID,
        Size{10.f, 10.f},
        Position(0.f, 10.f),
        Velocity(150.f, 0.f)
    ).entity();

    collision_start_count = 0;
    collision_end_count = 0;
    collision_last_target = {0, 0};

    world.listen<CollisionStart>(entity, onCollisionStart);
    world.listen<CollisionEnd>(entity, onCollisionEnd);

    world.progress();

    cr_assert_float_eq(world.get<Position>(entity)->x, 150.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->x, 150.f, 0.001f);
    cr_assert_eq(collision_start_count, 1);
    cr_assert_eq(collision_end_count, 0);
    cr_assert(collision_last_target == sensor);
}

Test(control, ground_sensor_sets_and_clears_is_on_ground) {
    ecs::World world;
    world.plugin<ControlPlugin>();

    world.deltaTime = 1.f;
    const ecs::Entity floor = world.create().set(
        RigidBody::RIGID,
        Size{100.f, 20.f},
        Position(0.f, 100.f),
        Velocity(0.f, 0.f)
    ).entity();
    const ecs::Entity entity = world.create().set(
        GroundSensorComponent{},
        RigidBody::RIGID,
        Size{10.f, 10.f},
        Position(10.f, 0.f),
        Velocity(0.f, 120.f)
    ).entity();

    cr_assert_not(floor == entity);
    cr_assert(!world.has<IsOnGround>(entity));

    world.progress();
    cr_assert(world.has<IsOnGround>(entity));

    world.set<Position>(entity, {10.f, 0.f});
    world.set<Velocity>(entity, {0.f, 0.f});
    world.progress();
    cr_assert(!world.has<IsOnGround>(entity));
}

Test(control, killing_grounded_entity_does_not_crash) {
    ecs::World world;
    world.plugin<ControlPlugin>();

    world.deltaTime = 1.f;
    world.create().set(
        RigidBody::RIGID,
        Size{100.f, 20.f},
        Position(0.f, 100.f),
        Velocity(0.f, 0.f)
    );
    const ecs::Entity entity = world.create().set(
        GroundSensorComponent{},
        RigidBody::RIGID,
        Size{10.f, 10.f},
        Position(10.f, 0.f),
        Velocity(0.f, 120.f)
    ).entity();

    world.progress();
    cr_assert(world.has<IsOnGround>(entity));

    world.kill(entity);
    cr_assert(!world.isAlive(entity));
}
