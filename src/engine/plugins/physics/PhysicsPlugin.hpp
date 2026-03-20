#pragma once
#include "SpatialQuery.hpp"
#include "engine/ecs/World.hpp"

enum RigidBody {
    SENSOR = 1u << 0u,
    RIGID = 1u << 1u
};

struct EmitCollisionEvent {
};

struct CollisionStart {
    ecs::Entity target;
};

struct CollisionEnd {
    ecs::Entity target;
};

struct Gravity {
    float scale = 1;
};

SYSTEM(IntegrateVelocitySys, With<Velocity, Position>, Without<RigidBody>, On<PostUpdate>) {
    ITER(view);
};

SYSTEM(GravitySys, With<Velocity, Gravity>, On<PostUpdate>) {
    ITER(view);
};

SYSTEM(PhysicsSys, With<Position, Size, Velocity, RigidBody>, On<PostUpdate>) {
    ITER(view);

    RUN(world) const;
};

struct PhysicsPlugin {
    static void load(ecs::World &world);

    static void unload(ecs::World &world);
};
