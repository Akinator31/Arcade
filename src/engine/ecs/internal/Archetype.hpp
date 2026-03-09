#pragma once
#include "ComponentRegistry.hpp"
#include "engine/datastructure/SparseIndices.hpp"
#include "engine/datastructure/SparseSet.hpp"
#include "engine/ecs/type.hpp"

#include <optional>

namespace ecs::internal {
    struct ArchetypeColumn {
        void *buffer = nullptr;
        std::uint16_t elementSize = 0;
    };


    class Archetype {
        EntityType type;
        datastructures::EcsVec<Entity, uint16_t> entities;
        // store at ComponentID: { buffer, element_size }
        datastructures::SparseSet<ArchetypeColumn, uint8_t> columns;

    public:
        datastructures::SparseIndices addEdge;
        datastructures::SparseIndices removeEdge;

        Archetype(EntityType type, const ComponentRegistry &componentRegistry);

        ~Archetype();

        Archetype(const Archetype &) = delete;

        Archetype &operator=(const Archetype &) = delete;

        Archetype(Archetype &&) noexcept = default;

        Archetype &operator=(Archetype &&) noexcept = default;

        [[nodiscard]] const EntityType &getType() const;

        internal::EntityRow addEntity(Entity);

        std::optional<Entity> removeEntity(internal::EntityRow row);

        [[nodiscard]] void *getComponent(const EntityRow row,
                                         const ComponentID component) const {
            auto &[buf, size] = this->columns.get(component);
            return static_cast<char *>(buf) + row * size;
        }

        void copyTo(const EntityRow row, const ComponentID component,
                    void *dest) const {
            auto &[buf, size] = this->columns.get(component);
            std::memcpy(dest, static_cast<char *>(buf) + row * size, size);
        }

        [[nodiscard]] void *getColumn(ComponentID component) const;

        [[nodiscard]] bool has(ComponentID component) const;

        [[nodiscard]] std::size_t count() const;

        [[nodiscard]] const Entity *getEntities() const;
    };
} // namespace ecs::internal
