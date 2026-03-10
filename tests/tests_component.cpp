#include "engine/ecs/World.hpp"
#include <criterion/criterion.h>


Test(component, default_constructor) {
    ecs::World world;

    struct Position {
        float x, y;

        Position() : x(10), y(10) {
        }
    };

    const ecs::Entity player = world.entity();

    world.add<Position>(player);

    cr_assert_eq(world.get<Position>(player)->x, 10);

    struct Health {
        uint32_t count = 0;

        Health() = default;

        explicit Health(ecs::World &world) : count(world.entity().index) {
        }
    };

    world.add<Health>(player);

    cr_assert_eq(world.get<Health>(player)->count, player.index + 1);
}

struct Hovered {
};

struct Clicked : Required<Hovered> {
};

Test(component, required) {
    ecs::World world;


    const ecs::Entity player = world.entity();

    world.add<Clicked>(player);
    cr_assert(world.has<Hovered>(player));
    cr_assert(world.has<Clicked>(player));
}
