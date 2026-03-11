#include "engine/ecs/World.hpp"
#include <criterion/criterion.h>

struct ChildOf {
};

Test(relation, simple) {
    ecs::World world;


    const ecs::Entity parent = world.entity();
    const ecs::Entity child = world.entity();

    cr_assert(!world.has_target<ChildOf>(child, parent));
    world.relate<ChildOf>(child, parent);
    cr_assert(world.has_target<ChildOf>(child, parent));
    cr_assert(world.has_source<ChildOf>(parent, child));
    world.unrelate<ChildOf>(child);
    cr_assert(!world.has<RelationSource<ChildOf>>(parent));
    cr_assert(!world.has<RelationTarget<ChildOf>>(child));
}


Test(relation, despawn) {
    ecs::World world;
    world.relation<ChildOf>();

    const ecs::Entity parent = world.entity();
    ecs::Entity child = world.entity();

    world.relate<ChildOf>(child, parent);
    cr_assert(world.has_source<ChildOf>(parent, child));
    world.kill(child);
    cr_assert(!world.has_source<ChildOf>(parent, child));

    child = world.entity();
    world.relate<ChildOf>(child, parent);
    cr_assert(world.has_source<ChildOf>(parent, child));
    world.kill(parent);
    cr_assert(!world.has_target<ChildOf>(child, parent));
}

Test(relation, iterRelated) {
    ecs::World world;
    world.relation<ChildOf>();

    const ecs::Entity root = world.entity();
    const ecs::Entity child1 = world.entity();
    const ecs::Entity child2 = world.entity();
    const ecs::Entity grandchild1 = world.entity();
    const ecs::Entity grandchild2 = world.entity();

    world.relate<ChildOf>(child1, root);
    world.relate<ChildOf>(child2, root);
    world.relate<ChildOf>(grandchild1, child1);
    world.relate<ChildOf>(grandchild2, child2);

    int count = 0;
    for (auto [parent, e] : world.iterRelated<ChildOf>(root)) {
        cr_assert(parent == root);
        cr_assert(e == child1 || e == child2);
        count++;
    }
    cr_assert(count == 2);

    int rec_count = 0;
    for (auto [parent, e] : world.iterRelated<ChildOf, true>(root)) {
        cr_assert(e == child1 || e == child2 || e == grandchild1 || e == grandchild2);
        if (e == child1 || e == child2) {
            cr_assert(parent == root);
        } else if (e == grandchild1) {
            cr_assert(parent == child1);
        } else if (e == grandchild2) {
            cr_assert(parent == child2);
        }
        rec_count++;
    }
    cr_assert(rec_count == 4);
}
