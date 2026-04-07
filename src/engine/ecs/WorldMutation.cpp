#include "World.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
    struct AddPlan {
        ecs::EntityType target_type;
        std::vector<ecs::ComponentID> added_components;
    };

#ifndef NDEBUG
    void validate_component_registered(const ecs::World &world, const ecs::ComponentID cid) {
        if (!world.component_registry.isRegistered(cid)) {
            std::string message = "component not registered: id=" + std::to_string(cid);
            if (cid < world.component_registry.components.size()) {
                const auto *name = world.component_registry.components[cid].name;
                if (name != nullptr) {
                    message += ", name=";
                    message += name;
                }
            }
            throw std::logic_error(message);
        }
    }
#endif

    void collect_required_components(ecs::World &world,
                                     ecs::EntityType &target_type,
                                     std::vector<ecs::ComponentID> &added_components,
                                     const ecs::ComponentID cid) {
        for (const ecs::ComponentID required_cid: world.component_registry.getRecord(cid).required) {
#ifndef NDEBUG
            validate_component_registered(world, required_cid);
#endif

            if (target_type.has(required_cid)) {
                continue;
            }
            target_type.add(required_cid);
            added_components.push_back(required_cid);
            collect_required_components(world, target_type, added_components, required_cid);
        }
    }

    AddPlan build_add_plan(ecs::World &world,
                           const ecs::internal::Archetype &arch,
                           const ecs::ComponentID *cid,
                           const uint32_t count) {
        AddPlan plan{arch.getType().clone(), {}};
        plan.added_components.reserve(count);

        for (uint32_t i = 0; i < count; i += 1) {
#ifndef NDEBUG
            validate_component_registered(world, cid[i]);
#endif
            if (plan.target_type.has(cid[i])) {
                continue;
            }
            plan.target_type.add(cid[i]);
            plan.added_components.push_back(cid[i]);
            collect_required_components(world, plan.target_type, plan.added_components, cid[i]);
        }

        return plan;
    }
} // namespace

namespace ecs {
    ArchetypeID World::findOrCreateArchetype(EntityType &&type) {
        auto [id, is_created] = this->archetype_registry.findOrCreateArchetype(std::move(type), *this);

        if (is_created) {
            for (const ComponentID cid: this->archetype_registry.getArchetype(id).getType()) {
                this->component_registry.getRecord(cid).archetypes.push_back(id);
            }
            for (QueryCache &query: this->queries) {
                query.update(*this, this->archetype_registry.getArchetype(id), id);
            }
        }

        return id;
    }

    void World::removeEntityOfArchetype(internal::Archetype &oldArch,
                                        const internal::EntityRow row) {
        if (const auto swapped = oldArch.removeEntity(row); swapped.has_value()) {
            auto &swappedRecord = this->entity_registry.getRecord(swapped.value());
            swappedRecord.row = row;
        }
    }

    void World::migrate(const Entity entity, const ArchetypeID newArchId) {
        auto &record = this->entity_registry.getRecord(entity);
        auto &newArch = this->archetype_registry.getArchetype(newArchId);

        const internal::EntityRow newRow = newArch.addEntity(entity);
        internal::Archetype &oldArch = this->archetype_registry.getArchetype(record.archetypeId);

        if (const auto &oldType = oldArch.getType(); oldType.count > 0) {
            for (const ComponentID cid: oldType) {
                if (oldArch.stores(cid) && newArch.stores(cid)) {
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

        if (arch.has(cid)) {
            return;
        }

        validate_component_registered(*this, cid);
        if (const auto &component_record = this->component_registry.getRecord(cid);
            component_record.required.empty()) {
            ArchetypeID newArchId;
            if (arch.addEdge.has(cid)) {
                newArchId = arch.addEdge.get(cid);
            } else {
                EntityType newType = arch.getType().clone();
                newType.add(cid);
                newArchId = this->findOrCreateArchetype(std::move(newType));
                this->archetype_registry.getArchetype(record.archetypeId).addEdge.set(cid, newArchId);
            }

            this->migrate(entity, newArchId);
            this->constructComponent(entity, cid);
            this->runOnAdd(entity, cid);
            this->runArchetypeOnAdd(entity);
            return;
        }

        auto [target_type, added_components] = build_add_plan(*this, arch, &cid, 1);
        const ArchetypeID newArchId = this->findOrCreateArchetype(std::move(target_type));
        this->archetype_registry.getArchetype(record.archetypeId).addEdge.set(cid, newArchId);

        this->migrate(entity, newArchId);
        for (const ComponentID added_cid: added_components) {
            this->constructComponent(entity, added_cid);
            this->runOnAdd(entity, added_cid);
        }
        this->runArchetypeOnAdd(entity);
    }

    bool World::add_id_batched(const Entity entity, const ComponentID *cid, const uint32_t count) {
        const auto &record = this->entity_registry.getRecord(entity);
        const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);

        auto [target_type, added_components] = build_add_plan(*this, arch, cid, count);
        if (added_components.empty()) {
            return false;
        }

        const ArchetypeID newArchId = this->findOrCreateArchetype(std::move(target_type));
        this->migrate(entity, newArchId);
        for (const ComponentID added_cid: added_components) {
            this->constructComponent(entity, added_cid);
            this->runOnAdd(entity, added_cid);
        }
        this->runArchetypeOnAdd(entity);
        return true;
    }

    void World::constructComponent(const Entity entity, const ComponentID cid) {
        if (const auto &record = this->component_registry.getRecord(cid); record.construct != nullptr) {
            record.construct(*this, this->get_id(entity, cid));
        }
    }

    void World::runOnAdd(const Entity entity, const ComponentID cid) {
        if (const auto &record = this->component_registry.getRecord(cid); record.onAdd != nullptr) {
            record.onAdd(*this, entity);
        }
    }

    void World::runArchetypeOnAdd(const Entity entity) {
        const auto &record = this->entity_registry.getRecord(entity);
        auto &arch = this->archetype_registry.getArchetype(record.archetypeId);

        for (const auto &sys: arch.onAdd) {
            sys(arch, record.row);
        }
    }

    void World::runOnRemove(const Entity entity, const ComponentID cid, const void *value) {
        if (const auto &record = this->component_registry.getRecord(cid); record.onRemove != nullptr &&
                                                                          value != nullptr) {
            record.onRemove(*this, entity, value);
        }
    }

    void World::remove_id(const Entity entity, const ComponentID cid) {
        const auto &record = this->entity_registry.getRecord(entity);
        auto &arch = this->archetype_registry.getArchetype(record.archetypeId);
        if (!arch.has(cid)) {
            return;
        }

        if (cid == reflection::type_id<Name>()) {
            this->clearEntityName(entity);
        }

        if (arch.stores(cid)) {
            const auto &onRemove = arch.columns.get(cid).onRemove;
            for (uint i = 0; i < onRemove.size; i += 1) {
                onRemove.data[i](arch, record.row);
            }
            this->runOnRemove(entity, cid, arch.getComponent(record.row, cid));
        }

        ArchetypeID newArchId;
        if (arch.removeEdge.has(cid)) {
            newArchId = arch.removeEdge.get(cid);
        } else {
            EntityType newType = arch.getType().clone();
            newType.remove(cid);
            newArchId = this->findOrCreateArchetype(std::move(newType));
            this->archetype_registry.getArchetype(record.archetypeId).removeEdge.set(cid, newArchId);
            this->archetype_registry.getArchetype(newArchId).addEdge.set(cid, record.archetypeId);
        }

        this->migrate(entity, newArchId);
    }

    void *World::get_id(const Entity entity, const ComponentID cid) {
        const auto &record = this->entity_registry.getRecord(entity);
        const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);
        if (!arch.has(cid) || !arch.stores(cid)) {
            return nullptr;
        }
        return arch.getComponent(record.row, cid);
    }

    const std::vector<internal::Archetype> &World::getArchetypes() const {
        return this->archetype_registry.archetypes;
    }

    std::vector<internal::Archetype> &World::getArchetypes() {
        return this->archetype_registry.archetypes;
    }

    void World::updateMatches(QueryCache &cached) {
        if (cached._required.count <= 0) {
            ArchetypeID i = 0;
            for (const internal::Archetype &archetype: this->getArchetypes()) {
                cached.update(*this, archetype, i);
                i += 1;
            }
            return;
        }

        for (const ComponentID first = cached._required.data[0];
             const ArchetypeID id: this->component_registry.getArchetypes(first)) {
            cached.update(*this, this->archetype_registry.getArchetype(id), id);
        }
    }
} // namespace ecs
