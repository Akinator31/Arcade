#include "engine/ecs/World.hpp"
#include <criterion/criterion.h>

struct TestPlugin {
    bool is_loaded = false;

    void load([[maybe_unused]] ecs::World &world) {
        is_loaded = true;
    }

    void unload([[maybe_unused]] ecs::World &world) {
        is_loaded = false;
    }
};

Test(plugin, lifecycle) {
    ecs::World world;

    cr_assert_not(world.hasPlugin<TestPlugin>());

    world.plugin<TestPlugin>();
    cr_assert(world.hasPlugin<TestPlugin>());

    auto *plugin = world.getPlugin<TestPlugin>();
    cr_assert_not_null(plugin);
    cr_assert(plugin->is_loaded);

    world.removePlugin<TestPlugin>();
    cr_assert_not(world.hasPlugin<TestPlugin>());
    cr_assert_not(plugin->is_loaded);
}

struct StatefulPlugin {
    int value;

    explicit StatefulPlugin(const int v) : value(v) {
    }

    void load([[maybe_unused]] ecs::World &world) {
        value += 10;
    }

    void unload([[maybe_unused]] ecs::World &world) {
        value -= 10;
    }
};

Test(plugin, stateful) {
    ecs::World world;

    world.plugin<StatefulPlugin>(5);

    const auto *plugin = world.getPlugin<StatefulPlugin>();
    cr_assert_not_null(plugin);
    cr_assert_eq(plugin->value, 15);

    world.removePlugin<StatefulPlugin>();
}

Test(plugin, auto_unload_on_destroy) {
    bool unloaded = false;

    struct AutoUnloadPlugin {
        bool *flag;

        explicit AutoUnloadPlugin(bool *f) : flag(f) {
        }

        static void load([[maybe_unused]] ecs::World &world) {
        }

        void unload([[maybe_unused]] ecs::World &world) const {
            *flag = true;
        }
    };

    {
        ecs::World world;
        world.plugin<AutoUnloadPlugin>(&unloaded);
    }

    cr_assert(unloaded);
}
