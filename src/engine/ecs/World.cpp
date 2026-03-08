#include "World.hpp"

namespace ecs {

World::World() : archetype_registry(this->component_registry) {

  [[maybe_unused]] auto archeId =
      this->archetype_registry.findOrCreateArchetype(EntityType());
}

void World::removeEntityOfArchetype(internal::Archetype &oldArch,
                                    const internal::EntityRow row) {
  if (auto swapped = oldArch.removeEntity(row); swapped.has_value()) {
    auto &swappedRecord = this->entity_registry.getRecord(swapped.value());
    swappedRecord.row = row;
  }
}

void World::migrate(const Entity entity, const ArchetypeID newArchId) {
  auto &record = this->entity_registry.getRecord(entity);
  auto &newArch = this->archetype_registry.getArchetype(newArchId);

  const internal::EntityRow newRow = newArch.addEntity(entity);

  internal::Archetype &oldArch =
      this->archetype_registry.getArchetype(record.archetypeId);

  if (const auto &oldType = oldArch.getType(); oldType.count > 0) {
    for (const ComponentID cid : oldType) {
      if (newArch.has(cid)) {
        oldArch.copyTo(record.row, cid, newArch.getComponent(newRow, cid));
      }
    }

    this->removeEntityOfArchetype(oldArch, record.row);
  }

  record.archetypeId = newArchId;
  record.row = newRow;
}

void World::add_id(const Entity entity, const ComponentID cid) {
  const auto &record = this->entity_registry.getRecord(entity);
  const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);

  if (arch.has(cid))
    return;

  ArchetypeID newArchId;
  if (arch.addEdge.has(cid)) {
    newArchId = arch.addEdge.get(cid);
  } else {
    EntityType newType = arch.getType().clone();
    newType.add(cid);
    newArchId =
        this->archetype_registry.findOrCreateArchetype(std::move(newType));
    this->archetype_registry.getArchetype(record.archetypeId)
        .addEdge.set(cid, newArchId);
  }
  this->migrate(entity, newArchId);
}

void World::remove_id(const Entity entity, const ComponentID cid) {
  const auto &record = this->entity_registry.getRecord(entity);
  const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);
  if (!arch.has(cid))
    return;

  ArchetypeID newArchId;
  if (arch.removeEdge.has(cid)) {
    newArchId = arch.removeEdge.get(cid);
  } else {
    EntityType newType = arch.getType().clone();
    newType.remove(cid);
    newArchId =
        this->archetype_registry.findOrCreateArchetype(std::move(newType));
    this->archetype_registry.getArchetype(record.archetypeId)
        .removeEdge.set(cid, newArchId);
    this->archetype_registry.getArchetype(newArchId).addEdge.set(
        cid, record.archetypeId);
  }
  this->migrate(entity, newArchId);
}

void *World::get_id(const Entity entity, const ComponentID cid) {
  const auto &record = this->entity_registry.getRecord(entity);
  const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);
  if (!arch.has(cid)) {
    return nullptr;
  }
  return arch.getComponent(record.row, cid);
}

} // namespace ecs