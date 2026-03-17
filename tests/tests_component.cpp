#include "engine/ecs/World.hpp"
#include <cstdlib>
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

Test(component, name_owns_allocated_string) {
    const char *literal = "Player1";
    const Name name{literal};

    cr_assert_str_eq(name.value, literal);
    cr_assert_neq(name.value, literal);

    free(const_cast<char *>(name.value));
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

static int managed_string_removed = 0;

struct ManagedString {
    const char *value;

    ManagedString() : value(strdup("")) {
    }

    explicit ManagedString(const char *input) : value(strdup(input)) {
    }

    static void onRemove(ecs::World &, ecs::Entity, const ManagedString *string) {
        managed_string_removed += 1;
        free(const_cast<char *>(string->value));
    }
};

Test(component, on_remove_hook_runs_on_set_remove_and_kill) {
    ecs::World world;
    const ecs::Entity entity = world.entity();

    managed_string_removed = 0;
    world.registerComponent<ManagedString>();

    world.set<ManagedString>(entity, ManagedString{"first"});
    cr_assert_eq(managed_string_removed, 1);

    world.set<ManagedString>(entity, ManagedString{"second"});
    cr_assert_eq(managed_string_removed, 2);

    world.remove<ManagedString>(entity);
    cr_assert_eq(managed_string_removed, 3);

    world.set<ManagedString>(entity, ManagedString{"third"});
    cr_assert_eq(managed_string_removed, 4);

    world.kill(entity);
    cr_assert_eq(managed_string_removed, 5);
}

Test(component, on_remove_hook_runs_on_world_destroy) {
    managed_string_removed = 0;

    {
        ecs::World world;
        const ecs::Entity entity = world.entity();
        world.registerComponent<ManagedString>();
        world.set<ManagedString>(entity, ManagedString{"persisted"});
        cr_assert_eq(managed_string_removed, 1);
    }

    cr_assert_eq(managed_string_removed, 2);
}
