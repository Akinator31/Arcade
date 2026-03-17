#include "engine/ecs/World.hpp"
#include "engine/plugins/render/PositionPropagationPlugin/PositionPropagationPlugin.hpp"
#include <criterion/criterion.h>

Test(position_propagation_plugin, simple) {
    ecs::World world;
    world.plugin<PositionPropagationPlugin>();

    auto a = world.create().set(Position{10, 10});
    cr_assert_eq(world.get<GlobalPosition>(a.entity())->x, 0);
    world.progress();
    cr_assert_eq(world.get<GlobalPosition>(a.entity())->x, 10);
}

Test(position_propagation_plugin, propagates_local_positions_through_hierarchy) {
    ecs::World world;
    world.plugin<PositionPropagationPlugin>();

    const ecs::Entity root = world.create().set(Position{10.f, 20.f}).id();
    const ecs::Entity child = world.create().set(Position{2.f, 3.f}).relate<Hierarchy>(root).id();
    const ecs::Entity leaf = world.create().set(Position{-1.f, 4.f}).relate<Hierarchy>(child).id();

    world.progress();

    const auto *root_global = world.get<GlobalPosition>(root);
    const auto *child_global = world.get<GlobalPosition>(child);
    const auto *leaf_global = world.get<GlobalPosition>(leaf);

    cr_assert_not_null(root_global);
    cr_assert_not_null(child_global);
    cr_assert_not_null(leaf_global);
    cr_assert_float_eq(root_global->x, 10.f, 0.001f);
    cr_assert_float_eq(root_global->y, 20.f, 0.001f);
    cr_assert_float_eq(child_global->x, 12.f, 0.001f);
    cr_assert_float_eq(child_global->y, 23.f, 0.001f);
    cr_assert_float_eq(leaf_global->x, 11.f, 0.001f);
    cr_assert_float_eq(leaf_global->y, 27.f, 0.001f);
}

Test(position_propagation_plugin, uses_root_global_position_when_no_local_position_exists) {
    ecs::World world;
    world.plugin<PositionPropagationPlugin>();

    const ecs::Entity root = world.entity();
    world.set<GlobalPosition>(root, GlobalPosition{100.f, 50.f});

    const ecs::Entity child = world.create().set(Position{3.f, 4.f}).relate<Hierarchy>(root).id();

    world.progress();

    const auto *root_global = world.get<GlobalPosition>(root);
    const auto *child_global = world.get<GlobalPosition>(child);

    cr_assert_not_null(root_global);
    cr_assert_not_null(child_global);
    cr_assert_float_eq(root_global->x, 100.f, 0.001f);
    cr_assert_float_eq(root_global->y, 50.f, 0.001f);
    cr_assert_float_eq(child_global->x, 103.f, 0.001f);
    cr_assert_float_eq(child_global->y, 54.f, 0.001f);
}

Test(position_propagation_plugin, unload_stops_position_updates) {
    ecs::World world;
    world.plugin<PositionPropagationPlugin>();

    const ecs::Entity root = world.create().set(Position{1.f, 1.f}).id();
    const ecs::Entity child = world.create().set(Position{2.f, 2.f}).relate<Hierarchy>(root).id();

    world.progress();
    cr_assert_float_eq(world.get<GlobalPosition>(child)->x, 3.f, 0.001f);
    cr_assert_float_eq(world.get<GlobalPosition>(child)->y, 3.f, 0.001f);

    world.removePlugin<PositionPropagationPlugin>();
    world.set<Position>(root, {10.f, 10.f});
    world.progress();

    cr_assert_float_eq(world.get<GlobalPosition>(child)->x, 3.f, 0.001f);
    cr_assert_float_eq(world.get<GlobalPosition>(child)->y, 3.f, 0.001f);
}
