#include "engine/ecs/World.hpp"

#include <criterion/criterion.h>

#include "engine/ecs/Query.hpp"

struct Position {
    float x, y;
};

struct Velocity : Position {
};


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

    world.add<Position>(entity);
    world.get<Position>(entity)->x = 10;
    const auto *pos = world.get<Position>(entity);

    cr_assert_eq(pos->x, 10);
}

Test(ecs, query) {
    ecs::World world;

    const ecs::Entity entity = world.entity();
    world.add<Position>(entity);

    world.fetch<Position>()
            .iter([entity](ArchetypeView &view) {
                cr_assert_eq(view.entities()->index, entity.index);
                view.column<Position>()->x += 10;
            });

    cr_assert_eq(world.get<Position>(entity)->x, 10);

    const ecs::QueryID qid = world.cache(query<Position>());

    world.read(qid).iter([entity](ArchetypeView &view) {
        cr_assert_eq(view.entities()->index, entity.index);
        view.column<Position>()->x += 10;
    });

    cr_assert_eq(world.get<Position>(entity)->x, 20);
}

Test(ecs, system) {
    struct MySystem {
        using with = All<Position>;
        using without = All<Player>;

        static void iter([[maybe_unused]] ArchetypeView &view) {
            auto *positions = view.column<Position>();

            for (uint i = 0; i < view.count(); i++) {
                positions[i].x += 10;
            }
        }
    };
    ecs::World world;

    const PhaseId Startup = world.createPhase();

    const SystemId sys = world.registerSystem<MySystem>(Startup);

    world.add<Position>(world.entity());

    world.runSystem(sys);

    world.fetch<Position>().iter([](ArchetypeView &view) {
        cr_assert_eq(view.column<Position>()->x, 10);
    });
}
