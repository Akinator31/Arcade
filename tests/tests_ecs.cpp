#include "engine/ecs/World.hpp"
#include "engine/ecs/internal/Archetype.hpp"

#include <criterion/criterion.h>

#include "../src/engine/ecs/system/Query.hpp"

struct Player {
};


Test(ecs, entity_creation) {
    ecs::World world;

    ecs::Entity entity = world.entity();

    cr_assert_eq(entity.index, 0);
    cr_assert_eq(entity.generation, 0);
    cr_assert_eq(world.isAlive(entity), true);
    world.kill(entity);
    cr_assert_eq(world.isAlive(entity), false);

    entity = world.entity();
    cr_assert_eq(entity.index, 0);
    cr_assert_eq(entity.generation, 1);
    entity = world.entity();
    cr_assert_eq(entity.index, 1);
    cr_assert_eq(entity.generation, 0);
}

Test(ecs, component) {
    ecs::World world;

    const ecs::Entity entity = world.entity();

    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.add<Position>(entity);
    world.get<Position>(entity)->x = 10;
    const auto *pos = world.get<Position>(entity);

    cr_assert_eq(pos->x, 10);
}

Test(ecs, query) {
    ecs::World world;

    const ecs::Entity entity = world.entity();
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.add<Position>(entity);

    world.fetch<Position>()
            .iter([entity](ArchetypeView &view) {
                cr_assert_eq(view.entities()->index, entity.index);

                view.column<Position>()->x += 10;
            });

    cr_assert_eq(world.get<Position>(entity)->x, 10);

    Query q;
    q.required<Position>();
    const ecs::QueryID qid = world.cache(std::move(q));

    world.read(qid).iter([entity](ArchetypeView &view) {
        cr_assert_eq(view.entities()->index, entity.index);
        view.column<Position>()->x += 10;
    });

    cr_assert_eq(world.get<Position>(entity)->x, 20);
}

Test(archetype, clone_entity_copies_stored_components) {
    struct Health {
        int value;
    };

    struct Damage {
        int value;
    };

    ecs::World world;
    world.registerComponent<Health>();
    world.registerComponent<Damage>();

    ecs::EntityType type;
    type.add(reflection::type_id<Health>());
    type.add(reflection::type_id<Damage>());

    ecs::internal::Archetype archetype(std::move(type), world);

    constexpr ecs::Entity source = {1, 0};
    const ecs::internal::EntityRow source_row = archetype.addEntity(source);
    static_cast<Health *>(archetype.getComponent(source_row, reflection::type_id<Health>()))->value = 42;
    static_cast<Damage *>(archetype.getComponent(source_row, reflection::type_id<Damage>()))->value = 7;

    constexpr ecs::Entity clone = {2, 0};
    const ecs::internal::EntityRow clone_row = archetype.cloneEntity(source_row, clone);

    cr_assert_eq(archetype.count(), 2);
    cr_assert_eq(archetype.getEntities()[clone_row].index, clone.index);
    cr_assert_eq(static_cast<Health *>(archetype.getComponent(clone_row, reflection::type_id<Health>()))->value, 42);
    cr_assert_eq(static_cast<Damage *>(archetype.getComponent(clone_row, reflection::type_id<Damage>()))->value, 7);
}

Test(ecs, clone_entity_copies_components_and_keeps_unique_name) {
    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.registerComponent<Velocity>();

    const ecs::Entity source = world.entity();
    world.set<Position>(source, {12, 34});
    world.set<Velocity>(source, {5, 6});
    world.set<Name>(source, Name{"Player"});

    const ecs::Entity clone = world.clone(source);

    cr_assert_neq(clone.index, source.index);
    cr_assert_eq(world.get<Position>(clone)->x, 12);
    cr_assert_eq(world.get<Position>(clone)->y, 34);
    cr_assert_eq(world.get<Velocity>(clone)->x, 5);
    cr_assert_eq(world.get<Velocity>(clone)->y, 6);
    cr_assert_str_neq(world.get<Name>(clone)->value, world.get<Name>(source)->value);
    cr_assert(world.findEntityByName("Player").has_value());
}

Test(ecs, clone_entity_clones_direct_hierarchy_children) {
    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.registerComponent<Velocity>();

    const ecs::Entity parent = world.entity();
    world.set<Position>(parent, {10, 20});
    world.set<Name>(parent, Name{"Parent"});

    const ecs::Entity child = world.entity();
    world.set<Position>(child, {1, 2});
    world.set<Velocity>(child, {3, 4});
    world.relate<Hierarchy>(child, parent);

    const ecs::Entity cloned_parent = world.clone(parent);

    cr_assert(world.has<Children>(cloned_parent));
    cr_assert_eq(world.get<Children>(cloned_parent)->entities.size, 1);

    const ecs::Entity cloned_child = world.get<Children>(cloned_parent)->entities[0];
    cr_assert(world.has<Parent>(cloned_child));
    cr_assert_eq(world.get<Parent>(cloned_child)->target.index, cloned_parent.index);
    cr_assert_eq(world.get<Position>(cloned_child)->x, 1);
    cr_assert_eq(world.get<Position>(cloned_child)->y, 2);
    cr_assert_eq(world.get<Velocity>(cloned_child)->x, 3);
    cr_assert_eq(world.get<Velocity>(cloned_child)->y, 4);
    cr_assert_neq(cloned_child.index, child.index);
}

Test(ecs, clone_entity_clones_nested_hierarchy_recursively) {
    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();

    const ecs::Entity root = world.entity();
    world.set<Position>(root, {0, 0});
    world.set<Name>(root, Name{"Root"});

    const ecs::Entity child = world.entity();
    world.set<Position>(child, {5, 6});
    world.relate<Hierarchy>(child, root);

    const ecs::Entity leaf = world.entity();
    world.set<Position>(leaf, {7, 8});
    world.relate<Hierarchy>(leaf, child);

    const ecs::Entity cloned_root = world.clone(root);

    cr_assert(world.has<Children>(cloned_root));
    cr_assert_eq(world.get<Children>(cloned_root)->entities.size, 1);

    const ecs::Entity cloned_child = world.get<Children>(cloned_root)->entities[0];
    cr_assert(world.has<Children>(cloned_child));
    cr_assert_eq(world.get<Children>(cloned_child)->entities.size, 1);

    const ecs::Entity cloned_leaf = world.get<Children>(cloned_child)->entities[0];
    cr_assert_eq(world.get<Parent>(cloned_child)->target.index, cloned_root.index);
    cr_assert_eq(world.get<Parent>(cloned_leaf)->target.index, cloned_child.index);
    cr_assert_eq(world.get<Position>(cloned_child)->x, 5);
    cr_assert_eq(world.get<Position>(cloned_child)->y, 6);
    cr_assert_eq(world.get<Position>(cloned_leaf)->x, 7);
    cr_assert_eq(world.get<Position>(cloned_leaf)->y, 8);
    cr_assert_neq(cloned_child.index, child.index);
    cr_assert_neq(cloned_leaf.index, leaf.index);
}

Test(ecs, create_constructible_entity_uses_props_and_default_props) {
    struct ConstructedEntity {
        struct Props {
            float x = 1.f;
            float y = 2.f;
        };

        static Props Default() {
            return Props { .x = 0, .y = 0};
        }

        static void construct(ecs::EntityRef ref, const Props props = {.x = 1, .y = 2}) {
            ref.set(Position{props.x, props.y});
        }
    };

    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();

    const ecs::EntityRef with_props = world.create<ConstructedEntity>({42.f, 24.f});
    cr_assert(world.isAlive(with_props.entity()));
    cr_assert_eq(world.get<Position>(with_props.entity())->x, 42.f);
    cr_assert_eq(world.get<Position>(with_props.entity())->y, 24.f);

    const ecs::EntityRef with_default_props = world.create<ConstructedEntity>();
    cr_assert(world.isAlive(with_default_props.entity()));
    cr_assert_eq(world.get<Position>(with_default_props.entity())->x, 1.f);
    cr_assert_eq(world.get<Position>(with_default_props.entity())->y, 2.f);
}