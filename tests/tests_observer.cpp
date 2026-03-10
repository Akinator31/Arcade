//
// Created by suleyman on 10/03/2026.
//

#include "engine/ecs/World.hpp"

#include <criterion/criterion.h>

#include "engine/ecs/Query.hpp"

struct Player {
};

Test(observer, base) {
    ecs::World world;

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
