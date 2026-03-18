#pragma once

#include "entity/type.hpp"

namespace ecs {
    class World;

    struct PluginFamily {
    };

    using PluginId = uint16_t;

    template<typename T>
    concept IsPlugin = requires(T plugin, World &world)
    {
        { plugin.load(world) };
        { plugin.unload(world) };
    };

    struct PluginRecord {
        void *instance = nullptr;
        void (*unload)(void *, World &) = nullptr;
        void (*destroy)(void *) = nullptr;
    };
}
