//
// Created by suleyman on 10/03/2026.
//

#include "engine/ecs/World.hpp"

#include <criterion/criterion.h>

#include "../src/engine/ecs/system/Query.hpp"

struct Player {
};

Test(observer, base) {
    ecs::World world;
    world.registerComponent<Player>();

    static uint32_t count = 0;
    struct OnAddPlayer : With<Player>, On<Add> {
        static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
            count += 1;
        };
    };
    struct OnRemovePlayer : With<Player>, On<Remove> {
        static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
            count -= 1;
        };
    };
    struct OnDespawnPlayer : With<Player>, On<Despawn> {
        static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
            count += 1;
        };
    };

    world.system<OnAddPlayer>();
    world.system<OnRemovePlayer>();
    world.system<OnDespawnPlayer>();

    const ecs::Entity player = world.entity();

    world.add<Player>(player);
    cr_assert_eq(count, 1);
    world.remove<Player>(player);
    cr_assert_eq(count, 0);
    world.add<Player>(player);
    cr_assert_eq(count, 1);
    world.kill(player);
    cr_assert_eq(count, 2);
}


Test(observer, already_created_table) {
    ecs::World world;
    world.registerComponent<Player>();

    const ecs::Entity player = world.entity();
    world.add<Player>(player);
    world.remove<Player>(player);

    static uint32_t count = 0;
    struct OnAddPlayer : With<Player>, On<Add> {
        static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
            count += 1;
        };
    };
    struct OnRemovePlayer : With<Player>, On<Remove> {
        static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
            count -= 1;
        };
    };
    struct OnDespawnPlayer : With<Player>, On<Despawn> {
        static void observe(ecs::internal::Archetype &, ecs::internal::EntityRow) {
            count += 1;
        };
    };

    world.system<OnAddPlayer>();
    world.system<OnRemovePlayer>();
    world.system<OnDespawnPlayer>();

    world.add<Player>(player);
    cr_assert_eq(count, 1);
}

Test(observer, remove_observer) {
    ecs::World world;
    world.registerComponent<Player>();

    static int count = 0;
    SYSTEM(OnAddPlayer, With<Player>, On<Add>) {
        OBSERVE(,) {
            count += 1;
        }
    };
    auto [pid, index] = world.system<OnAddPlayer>();
    const ecs::QueryID qid = world.getSystems(pid)[index].qid;


    cr_assert_eq(count, 0);
    cr_assert_eq(world.queries[qid].matches.size, 0);
    world.create().add<Player>();
    cr_assert_eq(world.queries[qid].matches.size, 1);
    cr_assert_eq(count, 1);
    world.remove<OnAddPlayer>();
    world.create().add<Player>();
    cr_assert_eq(count, 1);
}
