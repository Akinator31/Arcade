#include <criterion/criterion.h>

#include "engine/plugins/sprite/SpritePlugin.hpp"

Test(sprite_plugin, animates_and_loops_frames) {
    ecs::World world;
    world.plugin<SpritePlugin>();

    const ecs::Entity entity = world.entity();
    world.set<Sprite>(entity, {
        .texture = 0,
        .scale = {1.f, 1.f},
        .rect = {0, 0, 0, 0}
    });
    world.set<SpriteAtlas>(entity, {
        .tile_width = 16,
        .tile_height = 24,
        .rows = 2,
        .cols = 4
    });
    world.set<SpriteAnimation>(entity, {
        .timer = Timer(0.5f),
        .start = 1,
        .end = 3
    });

    world.deltaTime = 0.f;
    world.progress();
    auto *sprite = world.get<Sprite>(entity);
    cr_assert_eq(sprite->rect.left, 16);
    cr_assert_eq(sprite->rect.top, 0);
    cr_assert_eq(sprite->rect.width, 16);
    cr_assert_eq(sprite->rect.height, 24);

    world.deltaTime = 0.5f;
    world.progress();
    cr_assert_eq(sprite->rect.left, 32);
    cr_assert_eq(sprite->rect.top, 0);

    world.progress();
    cr_assert_eq(sprite->rect.left, 48);
    cr_assert_eq(sprite->rect.top, 0);

    world.progress();
    cr_assert_eq(sprite->rect.left, 16);
    cr_assert_eq(sprite->rect.top, 0);
}

Test(sprite_plugin, unload_stops_animation_updates) {
    ecs::World world;
    world.plugin<SpritePlugin>();

    const ecs::Entity entity = world.entity();
    world.set<Sprite>(entity, {
        .texture = 0,
        .scale = {1.f, 1.f},
        .rect = {0, 0, 0, 0}
    });
    world.set<SpriteAtlas>(entity, {
        .tile_width = 8,
        .tile_height = 8,
        .rows = 1,
        .cols = 4
    });
    world.set<SpriteAnimation>(entity, {
        .timer = Timer(0.25f),
        .start = 0,
        .end = 2
    });

    world.deltaTime = 0.f;
    world.progress();

    world.removePlugin<SpritePlugin>();
    world.deltaTime = 0.25f;
    world.progress();

    const auto *sprite = world.get<Sprite>(entity);
    cr_assert_eq(sprite->rect.left, 0);
    cr_assert_eq(sprite->rect.top, 0);
    cr_assert_eq(sprite->rect.width, 8);
    cr_assert_eq(sprite->rect.height, 8);
}
