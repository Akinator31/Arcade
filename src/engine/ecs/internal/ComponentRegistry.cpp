#include "ComponentRegistry.hpp"

namespace ecs::internal {

std::size_t ComponentRegistry::getSize(const ComponentID cid) const {
  if (cid >= this->components.size()) {
    return 0;
  }
  return this->components[cid].size;
}

void ComponentRegistry::registerComponent(const ComponentID cid, const std::size_t size) {
  if (cid < this->components.size() && this->components[cid].size != 0) {
    return;
  }
  if (cid >= this->components.size()) {
    this->components.resize(cid + 1);
  }
  this->components[cid].size = size;
}

ComponentRecord& ComponentRegistry::getRecord(const ComponentID cid) {
  return this->components[cid];
}

void ComponentRegistry::addArchetype(const ComponentID cid, const ArchetypeID archId) {
  if (cid >= this->components.size()) {
    this->components.resize(cid + 1);
  }
  this->components[cid].archetypes.push_back(archId);
}

void ComponentRegistry::addRequired(const ComponentID cid, const ecs::ComponentID requiredCid) {
  if (cid >= this->components.size()) {
    this->components.resize(cid + 1);
  }
  this->components[cid].required.push_back(requiredCid);
}

static constexpr std::vector<ArchetypeID> emptyArchetypes;

const std::vector<ArchetypeID>& ComponentRegistry::getArchetypes(const ComponentID cid) const {
  return this->components[cid].archetypes;
}
} // namespace ecs::internal
