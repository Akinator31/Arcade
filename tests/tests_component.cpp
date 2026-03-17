#include "engine/ecs/World.hpp"
#include <criterion/criterion.h>
#include <stdexcept>


Test(component, default_constructor) {
    ecs::World world;

    struct Position {
        float x, y;

        Position() : x(10), y(10) {
        }
    };

    const ecs::Entity player = world.entity();

    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.add<Position>(player);

    cr_assert_eq(world.get<Position>(player)->x, 10);

    struct Health {
        uint32_t count = 0;

        Health() = default;

        explicit Health(ecs::World &world) : count(world.entity().index) {
        }
    };

    world.registerComponent<Health>();
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

    world.registerComponent<Hovered>();
    world.registerComponent<Clicked>();
    world.add<Clicked>(player);
    cr_assert(world.has<Hovered>(player));
    cr_assert(world.has<Clicked>(player));
}

Test(component, name_default_on_add) {
    ecs::World world;

    const ecs::Entity entity = world.entity();
    world.registerComponent<Name>();
    world.add<Name>(entity);

    const std::string expected = "entity(" + std::to_string(entity.index) + ", " + std::to_string(entity.generation) +
                                 ")";

    cr_assert_str_eq(world.get<Name>(entity)->value, expected.c_str());

    const auto found = world.findEntityByName(expected);
    cr_assert(found.has_value());
    cr_assert(found.value() == entity);
}

Test(component, name_rejects_invalid_identifier) {
    bool thrown = false;

    try {
        static_cast<void>(Name{"1invalid"});
    } catch (const std::invalid_argument &) {
        thrown = true;
    }

    cr_assert(thrown);
}

Test(component, name_must_be_unique) {
    ecs::World world;

    const ecs::Entity first = world.entity();
    const ecs::Entity second = world.entity();

    world.registerComponent<Name>();
    world.set<Name>(first, Name{"Player1"});

    bool thrown = false;
    try {
        world.set<Name>(second, Name{"Player1"});
    } catch (const std::invalid_argument &) {
        thrown = true;
    }

    cr_assert(thrown);
}
