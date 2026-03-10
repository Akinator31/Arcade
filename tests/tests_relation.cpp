#include "engine/ecs/World.hpp"
#include <criterion/criterion.h>

Test(relation, simple) {
    ecs::World world;


    const ecs::Entity parent = world.entity();
    const ecs::Entity child = world.entity();

    cr_assert(!world.has_target<ChildOf>(child, parent));
    world.relate<ChildOf>(child, parent);
    cr_assert(world.has_target<ChildOf>(child, parent));
    cr_assert(world.has_source<ChildOf>(parent, child));
    world.unrelate<ChildOf>(child);
    cr_assert(!world.has<ecs::relation::RelationSource<ChildOf>>(parent));
    cr_assert(!world.has<ecs::relation::RelationTarget<ChildOf>>(child));
}


Test(relation, despawn) {
    ecs::World world;

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
