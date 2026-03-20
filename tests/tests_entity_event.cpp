//
// Created by suleyman on 10/03/2026.
//

#include "engine/ecs/World.hpp"

#include <criterion/criterion.h>

#include "../src/engine/ecs/system/Query.hpp"

struct Damaged {
    int value;
};

Test(entity_event, base) {
    ecs::World world;

    const ecs::Entity player = world.entity();

    static int count = 0;

    world.listen<Damaged>(player, [](auto &, auto, const Damaged value) {
        count += value.value;
    });

    cr_assert_eq(count, 0);
    world.emit(player, Damaged{10});
    cr_assert_eq(count, 10);
}

Test(entity_event, supports_multiple_listeners_and_targeted_unlisten) {
    ecs::World world;

    const ecs::Entity player = world.entity();
    int count_a = 0;
    int count_b = 0;

    const EventListenerId listener_a = world.listen<Damaged>(player, [&](auto &, auto, const Damaged value) {
        count_a += value.value;
    });
    const EventListenerId listener_b = world.listen<Damaged>(player, [&](auto &, auto, const Damaged value) {
        count_b += value.value * 2;
    });

    world.emit(player, Damaged{3});
    cr_assert_eq(count_a, 3);
    cr_assert_eq(count_b, 6);

    world.unlisten<Damaged>(player, listener_a);
    world.emit(player, Damaged{4});
    cr_assert_eq(count_a, 3);
    cr_assert_eq(count_b, 14);

    world.unlisten<Damaged>(player, listener_b);
    world.emit(player, Damaged{5});
    cr_assert_eq(count_a, 3);
    cr_assert_eq(count_b, 14);
}

Test(entity_event, listeners_are_removed_when_entity_is_killed) {
    ecs::World world;

    const ecs::Entity player = world.entity();
    int count = 0;

    world.listen<Damaged>(player, [&](auto &, auto, const Damaged value) {
        count += value.value;
    });

    world.kill(player);

    world.emit(player, Damaged{3});
    cr_assert_eq(count, 0);

    const ecs::Entity reused = world.entity();
    cr_assert_eq(reused.index, player.index);
    cr_assert_neq(reused.generation, player.generation);

    world.emit(reused, Damaged{5});
    cr_assert_eq(count, 0);
}
