#pragma once

#include <vector>
#include <tuple>

#include "World.hpp"


namespace ecs {
    class World;
}

template<typename Component>
concept HasDefaultConstructor = requires()
{
    { Component() };
};

template<typename Component>
concept HasFromWorldConstructor = requires(ecs::World &world)
{
    { Component(world) };
};


struct RequiredSlice {
    const uint8_t count = 0;
    const ecs::ComponentID *needed = nullptr;
};

struct Any {
};


template<typename T>
concept HasRequiredComponents = requires(ecs::World &world, ecs::Entity entity)
{
    { T::add(world, entity) };
};
