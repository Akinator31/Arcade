#pragma once
#include "Relation.hpp"
#include "engine/datastructures/EcsVec.hpp"
#include "engine/ecs/entity/type.hpp"
#include "arcade/Types.hpp"
#include "engine/reflection/type_id.hpp"

namespace ecs {
    class World;
}

template<typename... Components>
struct Required {
    static std::array<ecs::ComponentID, sizeof...(Components)> required_components() {
        return {reflection::type_id<Components>()...};
    }
};

template<typename T>
struct RelationSource {
    datastructures::EcsVec<ecs::Entity> entities;
};

template<typename T>
struct RelationTarget {
    ecs::Entity target;

    rayflect(value, {
             value->member<uint32_t>("index");
             value->member<uint32_t>("generation");
             }

    )
};

struct Hierarchy : ecs::DespawnRelated {
};

using Parent = RelationTarget<Hierarchy>;
using Children = RelationSource<Hierarchy>;

struct Name {
    const char *value;

    Name();

    explicit Name(const char *name);

    explicit Name(const std::string &);

    static void onAdd(ecs::World &world, ecs::Entity entity);

    static void onRemove(ecs::World &world, ecs::Entity entity, const Name *name);

    static void onSet(ecs::World &world, ecs::Entity entity, const Name *name);

    rayflect(Name,
             Name->member<const char *> ("value");
    )
};

// the local position from the hierarchy
struct Position : Required<GlobalPosition>, Vec2Reflect {
    float x, y;

    Position(const float x, const float y) : x(x), y(y) {
    }
};