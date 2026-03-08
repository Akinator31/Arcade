#pragma once
#include "internal/ArchetypeRegistry.hpp"
#include "internal/EntityRegistry.hpp"

namespace ecs {
    class World {
        internal::EntityRegistry entity_registry;
        internal::ComponentRegistry component_registry;
        internal::ArchetypeRegistry archetype_registry;

    public:
        World();
        Entity entity() { return this->entity_registry.create(); }

        void kill(const Entity entity) {
            return this->entity_registry.destroy(entity);
        }

        [[nodiscard]] bool isAlive(const Entity entity) {
            return this->entity_registry.isAlive(entity);
        }

        template <typename T>
        void add(const Entity entity) {
            this->component_registry.registerComponent<T>();
            this->add_id(entity, reflection::type_id<T>());
        }

        template <typename T>
        void remove(const Entity entity) {
            this->remove_id(entity, reflection::type_id<T>());
        }

        template <typename T>
        T* get(const Entity entity) {
            return static_cast<T*>(this->get_id(entity, reflection::type_id<T>()));
        }

        const std::vector<internal::Archetype>& getArchetypes() const;

    private:
        void removeEntityOfArchetype(internal::Archetype& oldArch,
                                     internal::EntityRow row);
        void add_id(Entity entity, ComponentID cid);
        void remove_id(Entity entity, ComponentID cid);
        void* get_id(Entity entity, ComponentID cid);

        void migrate(Entity, ArchetypeID newArchId);
    };
} // namespace ecs