#include "World.hpp"

#include <format>
#include <stdexcept>

namespace ecs {
    Entity World::entity() {
        const Entity entity = this->entity_registry.create();
        this->set(entity, Name(std::format("entity({}, {})", entity.index, entity.generation)));
        return entity;
    }

    Entity World::clone(const Entity entity) {
        const Entity newEntity = this->entity();
        const internal::EntityRecord &entityRecord = this->entity_registry.getRecord(entity);
        internal::EntityRecord &newEntityRecord = this->entity_registry.getRecord(newEntity);
        internal::Archetype &source_archetype = this->getArchetypes()[entityRecord.archetypeId];
        internal::Archetype &new_entity_archetype = this->getArchetypes()[newEntityRecord.archetypeId];

        this->clearEntityName(newEntity);
        for (const ComponentID cid: new_entity_archetype.getType()) {
            if (new_entity_archetype.stores(cid)) {
                this->runOnRemove(newEntity, cid, new_entity_archetype.getComponent(newEntityRecord.row, cid));
            }
        }
        this->removeEntityOfArchetype(new_entity_archetype, newEntityRecord.row);

        newEntityRecord.archetypeId = entityRecord.archetypeId;
        newEntityRecord.row = source_archetype.cloneEntity(entityRecord.row, newEntity);

        if (this->has<Children>(newEntity)) {
            this->get<Children>(newEntity)->entities.size = 0;
        }

        if (this->has<Name>(newEntity)) {
            Name *name = this->get<Name>(newEntity);
            name->value = strdup(ecs::World::makeEntityName(newEntity).c_str());
            this->syncEntityName(newEntity);
        }

        for (const auto &[parent, child]: this->iterRelated<Hierarchy, false>(entity)) {
            const Entity newChild = this->clone(child);
            this->relate<Hierarchy>(newChild, newEntity);
        }

        return newEntity;
    }

    EntityRef World::create() {
        return EntityRef(*this, this->entity());
    }

    std::string World::makeEntityName(const Entity entity) {
        return "entity(" + std::to_string(entity.index) + ", " + std::to_string(entity.generation) + ")";
    }

    void World::clearEntityName(const Entity entity) {
        std::erase_if(this->entity_name_to_entity, [entity](const auto &entry) {
            return entry.second == entity;
        });
    }

    void World::syncEntityName(const Entity entity) {
        Name *name = this->get<Name>(entity);
        if (name == nullptr) {
            return;
        }

        std::string value = name->value == nullptr ? "" : name->value;
        if (value.empty()) {
            value = ecs::World::makeEntityName(entity);
            free(const_cast<char *>(name->value));
            name->value = strdup(value.c_str());
        }

        const Name validated_name(value.c_str());
        free(const_cast<char *>(validated_name.value));

        if (const auto it = this->entity_name_to_entity.find(value);
            it != this->entity_name_to_entity.end() && it->second != entity) {
            throw std::invalid_argument("duplicate entity name");
        }

        this->clearEntityName(entity);
        this->entity_name_to_entity[value] = entity;
    }

    std::optional<Entity> World::findEntityByName(const std::string &name) const {
        if (const auto it = this->entity_name_to_entity.find(name); it != this->entity_name_to_entity.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void World::command(const std::function<void(World &)> &command) {
        this->commands.push_back(command);
    }

    void World::flush() {
        for (auto &command: this->commands) {
            command(*this);
        }
        this->commands.clear();
    }

    void World::kill(const Entity entity) {
        if (const internal::EntityRecord &record = this->entity_registry.getRecord(entity); record.archetypeId != 0) {
            if (this->has<Name>(entity)) {
                this->clearEntityName(entity);
            }

            internal::Archetype &arch = this->archetype_registry.getArchetype(record.archetypeId);
            for (const auto &sys: arch.onDespawn) {
                sys(arch, record.row);
            }

            const internal::EntityRecord &updated_record = this->entity_registry.getRecord(entity);
            internal::Archetype &updated_arch = this->archetype_registry.getArchetype(updated_record.archetypeId);
            for (const ComponentID cid: updated_arch.getType()) {
                if (updated_arch.stores(cid)) {
                    this->runOnRemove(entity, cid, updated_arch.getComponent(updated_record.row, cid));
                }
            }
            this->removeEntityOfArchetype(updated_arch, updated_record.row);
        }

        this->EventRegistry::unlistenAll(entity);
        this->GlobalEventRegistry::unlistenAll(entity);
        this->entity_registry.destroy(entity);
    }

    bool World::isAlive(const Entity entity) {
        return this->entity_registry.isAlive(entity);
    }
} // namespace ecs
