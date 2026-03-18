#pragma once
#include "Archetype.hpp"
#include "engine/ecs/type.hpp"

#include <unordered_map>
#include <vector>

struct EntityTypeHasher {
    size_t operator()(const ecs::EntityType &e) const noexcept;
};

namespace ecs::internal {
    class ComponentRegistry;

    class ArchetypeRegistry {
        std::unordered_map<EntityType, ArchetypeID, EntityTypeHasher> archetypes_map;
        ComponentRegistry &componentRegistry;

    public:
        std::vector<Archetype> archetypes;

        explicit ArchetypeRegistry(ComponentRegistry &componentRegistry);

        [[nodiscard]] std::pair<ArchetypeID, bool> findOrCreateArchetype(EntityType &&type, ecs::World &world);

        Archetype &getArchetype(ArchetypeID archetypeId);
    };
}
