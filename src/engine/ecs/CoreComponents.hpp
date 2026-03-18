#pragma once
#include "engine/reflection/rayflect.hpp"
#include "Relation.hpp"
#include "engine/datastructures/EcsVec.hpp"
#include "type.hpp"

namespace ecs {
    class World;
}

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
             })
};

struct Hierarchy : ecs::DespawnRelated {
};

using Parent = RelationTarget<Hierarchy>;
using Children = RelationSource<Hierarchy>;

struct Name {
    const char *value;

    Name();

    explicit Name(const char *name);

    static void onAdd(ecs::World &world, ecs::Entity entity);

    static void onSet(ecs::World &world, ecs::Entity entity, const Name *name);

    rayflect(Name,
             Name->member<const char*>("value");
    )
};
