#include "ArchetypeRegistry.hpp"
#include "engine/ecs/internal/Archetype.hpp"
#include "engine/ecs/World.hpp"

size_t EntityTypeHasher::operator()(const ecs::EntityType &e) const noexcept {
    constexpr uint64_t offset = 14695981039346656037ull;

    uint64_t hash = offset;

    const uint16_t *data = e.data;
    const size_t size = e.count;

    for (size_t i = 0; i < size; ++i) {
        constexpr uint64_t prime = 1099511628211ull;
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= prime;
    }

    return hash;
}

namespace ecs::internal {

    ArchetypeRegistry::ArchetypeRegistry(ComponentRegistry &componentRegistry)
        : componentRegistry(componentRegistry) {
    }

    // return archetype id and if just created
    std::pair<ArchetypeID, bool> ArchetypeRegistry::findOrCreateArchetype(EntityType &&type, World &world) {
        if (const auto it = this->archetypes_map.find(type);
            it != this->archetypes_map.end()) {
            return {it->second, false};
        }
        auto id = static_cast<ArchetypeID>(this->archetypes.size());
        this->archetypes_map.emplace(type.clone(), id);
        this->archetypes.emplace_back(std::move(type), world);
        return {id, true};
    }

    Archetype &ArchetypeRegistry::getArchetype(const ArchetypeID archetypeId) {
        return this->archetypes[archetypeId];
    }
} // namespace ecs::internal