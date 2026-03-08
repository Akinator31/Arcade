#pragma once

#include "../../datastructure/IndiceAlocator.hpp"
#include "../type.hpp"

namespace ecs::internal {
struct EntityRecord {
    uint32_t generation = 0;
    ArchetypeID archetypeId = 0;
    EntityRow row = 0;
};

class EntityRegistry {
    datastructures::IndicesAllocator<EntityRecord> entities;

  public:
  Entity create() {
    const uint32_t index = this->entities.alloc();
    const uint32_t generation = this->entities.get(index).generation;
    this->entities.set({.generation = generation}, index);
    return {.index = index, .generation = generation};
  }

  bool isAlive(const Entity entity) {
    return this->entities.get(entity.index).generation == entity.generation;
  }

  void destroy(const Entity entity) {
    this->entities.get(entity.index).generation += 1;
    this->entities.remove(entity.index);
  }

  EntityRecord& getRecord(const Entity entity) {
    return this->entities.get(entity.index);
  }
};

} // namespace ecs::internal
