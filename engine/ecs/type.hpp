#pragma once
#include <cstdint>
#include <cstdlib>

namespace ecs {

struct Entity {
    uint32_t index;
    uint32_t generation;

    bool operator==(const Entity& other) const noexcept {
        return this->index == other.index && this->generation == other.generation;
    }
} __attribute__((__packed__));

using ComponentID = uint16_t;
using ArchetypeID = uint16_t;
using QueryID = uint16_t;

class EntityType {
  public:
    ComponentID* data = nullptr;
    uint8_t count = 0;

    EntityType() = default;
    ~EntityType() {
        ::free(data);
    }
    EntityType(EntityType&& o) noexcept : data(o.data), count(o.count) {
        o.data = nullptr;
        o.count = 0;
    }

    EntityType& operator=(EntityType&& o) noexcept {
        if (this != &o) {
            ::free(data);
            data = o.data;
            count = o.count;
            o.data = nullptr;
            o.count = 0;
        }
        return *this;
    }

    EntityType(const EntityType&) = delete;
    EntityType& operator=(const EntityType&) = delete;

    [[nodiscard]] const ComponentID* begin() const {
        return data;
    }
    [[nodiscard]] const ComponentID* end() const {
        return data + count;
    }

    void sortComponents();

    [[nodiscard]] bool has(uint16_t component) const;
    void add(uint16_t component);
    void remove(uint16_t component);
    [[nodiscard]] bool operator==(const EntityType& other) const noexcept;
    [[nodiscard]] EntityType clone() const;
};

namespace internal {
using EntityRow = uint16_t;
}

} // namespace ecs
