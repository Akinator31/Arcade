#pragma once

#include "engine/ecs/World.hpp"
#include "engine/plugins/physics/PhysicsPlugin.hpp"

struct IsOnGround {
};

struct GroundSensorComponent : Required<EmitCollisionEvent> {
    uint32_t contacts = 0;
    EventListenerId collision_start_listener = 0;
    EventListenerId collision_end_listener = 0;

    static void installListeners(ecs::World &world, const ecs::Entity entity, GroundSensorComponent &sensor) {
        if (sensor.collision_start_listener != 0 || sensor.collision_end_listener != 0) {
            return;
        }

        auto *current_sensor = world.get<GroundSensorComponent>(entity);
        if (current_sensor == nullptr) {
            return;
        }

        sensor.collision_start_listener = world.listen<CollisionStart>(
            entity,
            [](ecs::World &world_ref, const ecs::Entity self, const CollisionStart &) {
                auto *ground_sensor = world_ref.get<GroundSensorComponent>(self);
                if (ground_sensor == nullptr) {
                    return;
                }

                ground_sensor->contacts += 1;
                world_ref.add<IsOnGround>(self);
            }
        );

        sensor.collision_end_listener = world.listen<CollisionEnd>(
            entity,
            [](ecs::World &world_ref, const ecs::Entity self, const CollisionEnd &) {
                auto *ground_sensor = world_ref.get<GroundSensorComponent>(self);
                if (ground_sensor == nullptr) {
                    return;
                }

                if (ground_sensor->contacts > 0) {
                    ground_sensor->contacts -= 1;
                }
                if (ground_sensor->contacts == 0) {
                    world_ref.remove<IsOnGround>(self);
                }
            }
        );
    }

    static void onAdd(ecs::World &, const ecs::Entity) {
    }

    static void onSet(ecs::World &world, const ecs::Entity entity, const GroundSensorComponent *) {
        auto *sensor = world.get<GroundSensorComponent>(entity);
        if (sensor == nullptr) {
            return;
        }
        installListeners(world, entity, *sensor);
    }

    static void onRemove(ecs::World &world, const ecs::Entity entity, const GroundSensorComponent *sensor) {
        if (sensor->collision_start_listener != 0) {
            world.unlisten<CollisionStart>(entity, sensor->collision_start_listener);
        }
        if (sensor->collision_end_listener != 0) {
            world.unlisten<CollisionEnd>(entity, sensor->collision_end_listener);
        }
        world.command([entity](ecs::World &world_ref) {
            if (world_ref.isAlive(entity)) {
                world_ref.remove<IsOnGround>(entity);
            }
        });
    }
};

struct ControlPlugin {
    void load(ecs::World &world) {
        world.plugin<PhysicsPlugin>();
        world.registerComponent<IsOnGround>();
        world.registerComponent<GroundSensorComponent>();
    }

    void unload(ecs::World &) {
    }
};
