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

