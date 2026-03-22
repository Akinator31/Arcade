//
// Created by suleyman on 10/03/2026.
//

#include "engine/ecs/World.hpp"

#include <criterion/criterion.h>

#include "../src/engine/ecs/system/Query.hpp"

struct Damaged {
    int value;
};

struct WindowResize {
    int width;
    int height;
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

Test(global_event, base) {
    ecs::World world;
    int width = 0;
    int height = 0;

    world.globalListen<WindowResize>([&](auto &, const WindowResize value) {
        width += value.width;
        height += value.height;
    });

    world.globalEmit(WindowResize{1280, 720});
    cr_assert_eq(width, 1280);
    cr_assert_eq(height, 720);
}

Test(global_event, supports_multiple_listeners_and_targeted_unlisten) {
    ecs::World world;
    int count_a = 0;
    int count_b = 0;

    const EventListenerId listener_a = world.globalListen<WindowResize>([&](auto &, const WindowResize value) {
        count_a += value.width;
    });
    const EventListenerId listener_b = world.globalListen<WindowResize>([&](auto &, const WindowResize value) {
        count_b += value.height;
    });

    world.globalEmit(WindowResize{3, 4});
    cr_assert_eq(count_a, 3);
    cr_assert_eq(count_b, 4);

    world.globalUnlisten<WindowResize>(listener_a);
    world.globalEmit(WindowResize{5, 6});
    cr_assert_eq(count_a, 3);
    cr_assert_eq(count_b, 10);

    world.globalUnlisten<WindowResize>(listener_b);
    world.globalEmit(WindowResize{7, 8});
    cr_assert_eq(count_a, 3);
    cr_assert_eq(count_b, 10);
}

Test(global_event, world_listener_survives_other_entity_kill) {
    ecs::World world;
    const ecs::Entity entity = world.entity();
    int count = 0;

    world.globalListen<WindowResize>([&](auto &, const WindowResize value) {
        count += value.width;
    });

    world.kill(entity);
    world.globalEmit(WindowResize{9, 0});
    cr_assert_eq(count, 9);
}

Test(global_event, owned_listeners_are_removed_when_entity_is_killed) {
    ecs::World world;
    ecs::EntityRef owner = world.create();
    int count = 0;

    owner.globalListen<WindowResize>([&](auto &, const WindowResize value) {
        count += value.width;
    });

    world.globalEmit(WindowResize{2, 0});
    cr_assert_eq(count, 2);

    world.kill(owner.entity());
    world.globalEmit(WindowResize{3, 0});
    cr_assert_eq(count, 2);

    const ecs::Entity reused = world.entity();
    cr_assert_eq(reused.index, owner.index);
    cr_assert_neq(reused.generation, owner.generation);

    world.globalListen<WindowResize>(reused, [&](auto &, const WindowResize value) {
        count += value.height;
    });

    world.globalEmit(WindowResize{4, 5});
    cr_assert_eq(count, 7);
}
