#include "engine/ecs/World.hpp"

#include <criterion/criterion.h>

#include "engine/ecs/Query.hpp"

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

    world.registerComponent<Position>();
    world.add<Position>(entity);
    world.get<Position>(entity)->x = 10;
    const auto *pos = world.get<Position>(entity);

    cr_assert_eq(pos->x, 10);
}

Test(ecs, query) {
    ecs::World world;

    const ecs::Entity entity = world.entity();
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
