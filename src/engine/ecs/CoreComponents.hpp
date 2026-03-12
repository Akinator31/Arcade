#pragma once
#include "engine/reflection/rayflect.hpp"
#include "Relation.hpp"
#include "engine/datastructures/EcsVec.hpp"
#include "type.hpp"

template<typename T>
struct RelationSource {
    datastructures::EcsVec<ecs::Entity> entities;
};

template<typename T>
struct RelationTarget {
    ecs::Entity target;
};

struct Hierarchy : ecs::DespawnRelated {
};

using Parent = RelationTarget<Hierarchy>;
using Children = RelationSource<Hierarchy>;

struct Name {
    const char *value;

    rayflect(Name,
             Name->member<const char*>("value");
    )
};