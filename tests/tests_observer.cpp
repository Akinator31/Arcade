//
// Created by suleyman on 10/03/2026.
//

#include "engine/ecs/World.hpp"

#include <criterion/criterion.h>

#include "../src/engine/ecs/system/Query.hpp"

struct Player {
};

struct ScoreValue {
    int value;
};

struct BaseImage {
    int value;
};

struct HoverImage {
    int value;
};

struct HoveredTag {
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

Test(observer, on_add_after_set_sees_assigned_value) {
    ecs::World world;
    world.registerComponent<ScoreValue>();

    static int seen = -1;
    SYSTEM(OnAddScoreValue, With<ScoreValue>, On<Add>) {
        OBSERVE(table, row) {
            auto *score = static_cast<ScoreValue *>(table.getComponent(row, reflection::type_id<ScoreValue>()));
            seen = score->value;
        }
    };

    world.system<OnAddScoreValue>();

    const ecs::Entity entity = world.entity();
    seen = -1;

    world.set<ScoreValue>(entity, {.value = 42});

    cr_assert_eq(seen, 42);
}

Test(observer, on_add_after_migration_sees_existing_components) {
    ecs::World world;
    world.registerComponent<BaseImage>();
    world.registerComponent<HoverImage>();
    world.registerComponent<HoveredTag>();

    static int seen_base = -1;
    static int seen_hover = -1;
    SYSTEM(OnHoverAdded, With<HoveredTag, BaseImage, HoverImage>, On<Add>) {
        OBSERVE(table, row) {
            auto *base = static_cast<BaseImage *>(table.getComponent(row, reflection::type_id<BaseImage>()));
            auto *hover = static_cast<HoverImage *>(table.getComponent(row, reflection::type_id<HoverImage>()));
            seen_base = base->value;
            seen_hover = hover->value;
        }
    };

    world.system<OnHoverAdded>();

    const ecs::Entity entity = world.entity();
    world.set<BaseImage>(entity, {.value = 1});
    world.set<HoverImage>(entity, {.value = 2});

    seen_base = -1;
    seen_hover = -1;
    world.add<HoveredTag>(entity);

    cr_assert_eq(seen_base, 1);
    cr_assert_eq(seen_hover, 2);
}
