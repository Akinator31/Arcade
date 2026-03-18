#pragma once
#include "engine/datastructures/SparseIndices.hpp"
#include "engine/datastructures/SparseSet.hpp"
#include "../entity/type.hpp"

#include <optional>

namespace ecs {
    class World;
}

namespace ecs::internal {
    class Archetype;
}

using ObserverFunc = void(*)(ecs::internal::Archetype &, ecs::internal::EntityRow row);

namespace ecs::internal {
    struct ArchetypeColumn {
        void *buffer = nullptr;
        std::uint16_t elementSize = 0;

        struct {
            ObserverFunc *data = nullptr;
            uint8_t size = 0;
            uint8_t capacity = 0;

            void push_back(const ObserverFunc value) {
                if (this->size >= this->capacity) {
                    this->capacity = this->capacity == 0 ? 1 : this->capacity * 2;
                    this->data = static_cast<ObserverFunc *>(
                        realloc(this->data, this->capacity * sizeof(ObserverFunc)));
                }
                std::memcpy(this->data + this->size, &value, sizeof(ObserverFunc));
                this->size += 1;
            }

            void remove(const ObserverFunc value) {
                for (uint8_t i = 0; i < this->size; i++) {
                    if (this->data[i] == value) {
                        const uint8_t last = this->size - 1;
                        if (i != last) {
                            std::memcpy(this->data + i, this->data + last, sizeof(ObserverFunc));
                        }
                        this->size -= 1;
                        return;
                    }
                }
            }
        } onRemove;
    };


    class Archetype {
        EntityType type;
        datastructures::EcsVec<Entity, uint16_t> entities;
        // store at ComponentID: { buffer, element_size }

    public:
        ecs::World &world;

        datastructures::SparseSet<ArchetypeColumn, uint8_t> columns;

        datastructures::SparseIndices addEdge;
        datastructures::SparseIndices removeEdge;

        datastructures::EcsVec<ObserverFunc> onAdd;
        datastructures::EcsVec<ObserverFunc> onDespawn;

        Archetype(EntityType &&type, ecs::World &world);

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
            auto &[buf, size, _] = this->columns.get(component);
            return static_cast<char *>(buf) + row * size;
        }

        void copyTo(const EntityRow row, const ComponentID component,
                    void *dest) const {
            auto &[buf, size, _] = this->columns.get(component);
            std::memcpy(dest, static_cast<char *>(buf) + row * size, size);
        }

        [[nodiscard]] void *getColumn(ComponentID component) const;

        [[nodiscard]] bool has(ComponentID component) const;
        [[nodiscard]] bool stores(ComponentID component) const;

        [[nodiscard]] std::size_t count() const;

        [[nodiscard]] const Entity *getEntities() const;
    };
} // namespace ecs::internal
