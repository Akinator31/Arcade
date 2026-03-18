#include <criterion/criterion.h>
#include "engine/ecs/World.hpp"

struct CompA {
    int a = 1;
};

struct CompB {
    float b = 2.0f;
};

struct CompC {
    double c = 3.0;
};

Test(ecs_batch, add_batched) {
    ecs::World world;
    ecs::Entity e = world.entity();

    size_t first_count = world.getArchetypes().size();
    world.add<CompA, CompB, CompC>(e);
    size_t after_count = world.getArchetypes().size();

    cr_assert_eq(after_count, first_count + 1);

    cr_assert(world.has<CompA>(e));
    cr_assert(world.has<CompB>(e));
    cr_assert(world.has<CompC>(e));

    cr_assert_eq(world.get<CompA>(e)->a, 1);
    cr_assert_eq(world.get<CompB>(e)->b, 2.0f);
    cr_assert_eq(world.get<CompC>(e)->c, 3.0);
}
