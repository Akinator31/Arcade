#pragma once

namespace ecs {
    class World;
    struct Entity;
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
    { T::required_components() };
};

template<typename T>
concept HasOnAdd = requires(ecs::World &world, ecs::Entity entity)
{
    { T::onAdd(world, entity) };
};

template<typename T>
concept HasOnRemove = requires(ecs::World &world, ecs::Entity entity, const T *value)
{
    { T::onRemove(world, entity, value) };
};


template<typename T>
concept HasOnSet = requires(ecs::World &world, ecs::Entity entity, const T *value)
{
    { T::onSet(world, entity, value) };
};