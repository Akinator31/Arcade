#include "engine/ecs/World.hpp"
#include <criterion/criterion.h>

#include "plugins/DefaultPlugins.hpp"
#include "plugins/render/UiPlugin/UiPlugin.hpp"

struct Player {
};

struct Enemy {
};

Test(system, basic_execution) {
    struct MovementSystem : With<Position, Velocity> {
        static void iter(ArchetypeView &view) {
            auto *positions = view.column<Position>();
            const auto *velocities = view.column<Velocity>();

            for (uint32_t i = 0; i < view.count(); i++) {
                positions[i].x += velocities[i].x;
                positions[i].y += velocities[i].y;
            }
        }
    };

    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.registerComponent<Velocity>();

    const SystemId sys = world.system<MovementSystem>();

    const ecs::Entity e1 = world.entity();
    world.add<Position>(e1);
    world.add<Velocity>(e1);
    world.get<Position>(e1)->x = 0;
    world.get<Position>(e1)->y = 0;
    world.get<Velocity>(e1)->x = 1.5f;
    world.get<Velocity>(e1)->y = 2.0f;

    world.runSystem(sys);

    cr_assert_float_eq(world.get<Position>(e1)->x, 1.5f, 0.001f);
    cr_assert_float_eq(world.get<Position>(e1)->y, 2.0f, 0.001f);
}

Test(system, empty) {
    ecs::World world;

    static int count = 0;

    struct Empty {
        void run(ecs::World &) {
            count += 1;
        }
    };

    world.system<Empty>();

    world.progress();

    cr_assert_eq(count, 1);
}

struct AllowCondition {
    static bool condition(ecs::World &) {
        return true;
    }
};

struct DisallowCondition {
    static bool condition(ecs::World &) {
        return false;
    }
};

Test(system, condition) {
    ecs::World world;

    static int count = 0;


    struct Allowed : AllowCondition {
        void run(ecs::World &) {
            count += 1;
        }
    };

    struct Disallowed : DisallowCondition {
        void run(ecs::World &) {
            count += 1;
        }
    };
    world.progress();
    world.system<Allowed>();
    cr_assert_eq(count, 0);
    world.progress();
    cr_assert_eq(count, 1);
    world.remove<Allowed>();
    world.system<Disallowed>();
    world.progress();
    cr_assert_eq(count, 1);
    world.remove<Disallowed>();
    cr_assert_eq(count, 1);
}

Test(system, multiple_condition) {
    ecs::World world;

    static int count = 0;

    struct TwoAllow : Conditions<AllowCondition, AllowCondition> {
        void run(ecs::World &) {
            count += 1;
        }
    };

    world.system<TwoAllow>();

    world.progress();
    cr_assert_eq(count, 1);

    world.remove<TwoAllow>();
    struct AllowDisallow : Conditions<AllowCondition, DisallowCondition> {
        void run(ecs::World &) {
            count += 1;
        }
    };

    world.system<AllowDisallow>();

    world.progress();
    cr_assert_eq(count, 1);
}

Test(system, member_run_keeps_state) {
    ecs::World world;
    static int seen = 0;

    struct StatefulTask {
        int count = 0;

        RUN(world) {
            (void) world;
            count += 1;
            seen = count;
        }
    };

    world.system<StatefulTask>();

    world.progress();
    world.progress();

    cr_assert_eq(seen, 2);
}

Test(system, exclude_component) {
    struct PlayerOnlySystem : With<Position>, Without<Enemy> {
        static void iter(ArchetypeView &view) {
            auto *positions = view.column<Position>();
            for (uint32_t i = 0; i < view.count(); i++) {
                positions[i].x += 10.0f;
            }
        }
    };

    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.registerComponent<Enemy>();

    const SystemId sys = world.system<PlayerOnlySystem>();

    const ecs::Entity p1 = world.entity();
    world.add<Position>(p1);
    world.get<Position>(p1)->x = 0;

    const ecs::Entity e1 = world.entity();
    world.add<Position>(e1);
    world.add<Enemy>(e1);
    world.get<Position>(e1)->x = 0;

    world.runSystem(sys);

    cr_assert_float_eq(world.get<Position>(p1)->x, 10.0f, 0.001f);
    cr_assert_float_eq(world.get<Position>(e1)->x, 0.0f, 0.001f);
}

Test(system, multiple_archetypes) {
    struct GlobalGravitySystem : With<Velocity>, On<Update> {
        static void iter(ArchetypeView &view) {
            auto *velocities = view.column<Velocity>();
            for (uint32_t i = 0; i < view.count(); i++) {
                velocities[i].y -= 9.8f;
            }
        }
    };

    ecs::World world;
    world.registerComponent<Velocity>();
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.registerComponent<Player>();
    world.phase<Update>();
    const SystemId sys = world.system<GlobalGravitySystem>();

    const ecs::Entity e1 = world.entity();
    world.add<Velocity>(e1);
    world.get<Velocity>(e1)->y = 0.0f;

    const ecs::Entity e2 = world.entity();
    world.add<Velocity>(e2);
    world.add<Position>(e2);
    world.get<Velocity>(e2)->y = 10.0f;

    const ecs::Entity e3 = world.entity();
    world.add<Velocity>(e3);
    world.add<Player>(e3);
    world.get<Velocity>(e3)->y = -5.0f;

    world.runSystem(sys);

    cr_assert_float_eq(world.get<Velocity>(e1)->y, -9.8f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(e2)->y, 0.2f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(e3)->y, -14.8f, 0.001f);
}

Test(system, different_phases) {
    struct SystemA : With<Position>, On<PreUpdate> {
        static void iter(ArchetypeView &view) {
            auto *positions = view.column<Position>();
            for (uint32_t i = 0; i < view.count(); i++) positions[i].x += 1.0f;
        }
    };

    struct SystemB : With<Position>, On<PostUpdate> {
        static void iter(ArchetypeView &view) {
            auto *positions = view.column<Position>();
            for (uint32_t i = 0; i < view.count(); i++) positions[i].x *= 2.0f;
        }
    };

    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();


    const SystemId sysA = world.system<SystemA>();
    const SystemId sysB = world.system<SystemB>();

    const ecs::Entity e = world.entity();
    world.add<Position>(e);
    world.get<Position>(e)->x = 2.0f;

    world.runSystem(sysA);
    cr_assert_float_eq(world.get<Position>(e)->x, 3.0f, 0.001f);

    world.runSystem(sysB);
    cr_assert_float_eq(world.get<Position>(e)->x, 6.0f, 0.001f);
}

Test(system, remove_system) {
    struct SystemC : With<Position> {
        static void iter(ArchetypeView &view) {
            auto *positions = view.column<Position>();
            for (uint32_t i = 0; i < view.count(); i++) positions[i].x += 5.0f;
        }
    };

    ecs::World world;
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();

    world.system<SystemC>();

    const ecs::Entity e = world.entity();
    world.add<Position>(e);
    world.get<Position>(e)->x = 0.0f;

    world.progress();
    cr_assert_float_eq(world.get<Position>(e)->x, 5.0f, 0.001f);

    world.remove<SystemC>();

    world.progress();
    cr_assert_float_eq(world.get<Position>(e)->x, 5.0f, 0.001f);
}


Test(system, interval) {
    ecs::World world;
    static int count = 0;

    struct Sys : Interval<1000> {
        static void run(Sys *, ecs::World &) {
            count += 1;
        }
    };

    world.system<Sys>();
    world.deltaTime = 0.6;
    world.progress();
    cr_assert_eq(count, 0);
    world.progress();
    cr_assert_eq(count, 1);
    world.progress();
    cr_assert_eq(count, 1);
    world.progress();
    cr_assert_eq(count, 2);
}