#pragma once

#include "engine/ecs/type.hpp"
#include "engine/reflection/type_id.hpp"
#include <cstddef>
#include <vector>

namespace ecs {
class World;
}

namespace ecs::internal {

struct ComponentRecord {
    std::size_t size = 0;
    std::vector<ecs::ArchetypeID> archetypes;
    std::vector<ecs::ComponentID> required;
    void (*onAdd)(World&, Entity) = nullptr;
    void (*onRemove)(World&, Entity) = nullptr;
};

class ComponentRegistry {
    std::vector<ComponentRecord> components;

  public:
    [[nodiscard]] std::size_t getSize(ecs::ComponentID cid) const;
    void registerComponent(ecs::ComponentID cid, std::size_t size);
    void addArchetype(ecs::ComponentID cid, ecs::ArchetypeID archId);
    ComponentRecord& getRecord(ecs::ComponentID cid);

    [[nodiscard]] const std::vector<ecs::ArchetypeID>& getArchetypes(ecs::ComponentID cid) const;

    void addRequired(ecs::ComponentID cid, ecs::ComponentID requiredCid);

    template <typename T> void registerComponent() {
        this->registerComponent(reflection::type_id<T>(), sizeof(T));
    }
};

} // namespace ecs::internal
