#include "Archetype.hpp"

namespace ecs::internal {
Archetype::Archetype(EntityType type,
                     const ComponentRegistry &componentRegistry)
    : type(std::move(type)) {
  for (const auto &component : this->type) {
    std::size_t size = componentRegistry.getSize(component);
    if (size == 0)
      continue;
    this->columns.set(
        component,
        {malloc(size), static_cast<std::uint16_t>(size)});
  }
}

Archetype::~Archetype() {
  for (std::size_t i = 0; i < this->columns.size(); i++) {
    auto &[buf, size] = this->columns[i];
    free(buf);
  }
}

EntityRow Archetype::addEntity(const Entity entity) {
  const uint32_t count = this->entities.size;

  const bool needs_realloc = count >= this->entities.capacity;
  this->entities.push_back(entity);
  if (needs_realloc) {
    const uint32_t newCapacity = this->entities.capacity;
    for (std::size_t i = 0; i < this->columns.size(); i++) {
      auto &[buf, size] = this->columns[i];
      buf = realloc(buf, size * newCapacity);
      std::memset(static_cast<char *>(buf) + count * size, 0,
                  (newCapacity - count) * size);
    }
  }
  return static_cast<EntityRow>(count);
}

std::optional<Entity> Archetype::removeEntity(const EntityRow row) {
  const std::size_t last = this->entities.size - 1;
  std::optional<Entity> swapped = std::nullopt;

  if (static_cast<std::size_t>(row) != last) {
    for (std::size_t i = 0; i < this->columns.size(); i++) {
      auto &[buf, size] = this->columns[i];
      auto *base = static_cast<char *>(buf);
      std::memcpy(base + row * size, base + last * size, size);
    }
    swapped = this->entities[last];
    this->entities[row] = this->entities[last];
  }

  this->entities.pop_back();
  return swapped;
}

void *Archetype::getColumn(const ComponentID component) const {
  return this->columns.get(component).buffer;
}

const EntityType &Archetype::getType() const { return this->type; }

std::size_t Archetype::count() const { return this->entities.size; }

const Entity *Archetype::getEntities() const { return this->entities.data; }

bool Archetype::has(const ComponentID component) const {
  return this->columns.has(component);
}
} // namespace ecs::internal
