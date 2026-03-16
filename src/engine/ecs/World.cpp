#include "World.hpp"

#include <algorithm>
#include <stdexcept>

namespace ecs {
    World::World() : component_registry(), archetype_registry(this->component_registry) {
        [[maybe_unused]] auto archeId =
                this->findOrCreateArchetype({});

        this->registerComponent<Name>();
        this->relation<Hierarchy>();
    }

    World::~World() {
        for (auto &[instance, unload, destroy]: this->loaded_plugins) {
            if (instance) {
                unload(instance, *this);
                destroy(instance);
            }
        }
        for (auto &[systems]: this->phases) {
            for (auto &[id, qid, iter, value, run, destroy, condition]: systems) {
                if (destroy) {
                    destroy(value);
                }
            }
        }
    }

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

        internal::Archetype &oldArch =
                this->archetype_registry.getArchetype(record.archetypeId);

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

    bool World::add_id(const Entity entity, const ComponentID cid) {
#ifndef NDEBUG
        if (!this->component_registry.isRegistered(cid)) {
            throw std::logic_error("component not registered");
        }
#endif

        const auto &record = this->entity_registry.getRecord(entity);
        const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);

        if (arch.has(cid))
            return false;

        ArchetypeID newArchId;
        if (arch.addEdge.has(cid)) {
            newArchId = arch.addEdge.get(cid);
        } else {
            EntityType newType = arch.getType().clone();
            newType.add(cid);

            newArchId =
                    this->findOrCreateArchetype(std::move(newType));
            this->archetype_registry.getArchetype(record.archetypeId)
                    .addEdge.set(cid, newArchId);
        }
        this->migrate(entity, newArchId);
        this->constructComponent(entity, cid);
        this->runOnAdd(entity, cid);
        this->addRequiredComponents(entity, cid);
        return true;
    }

    bool World::add_id_batched(const Entity entity, const ComponentID *cid, const uint32_t count) {
        const auto &record = this->entity_registry.getRecord(entity);
        const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);


        EntityType newType = arch.getType().clone();
        std::vector<ComponentID> added;
        added.reserve(count);
        for (uint32_t i = 0; i < count; i++) {
            if (!this->component_registry.isRegistered(cid[i])) {
                throw std::logic_error("component not registered");
            }
            if (!arch.has(cid[i])) {
                newType.add(cid[i]);
                added.push_back(cid[i]);
            }
        }

        if (newType.count == arch.getType().count) {
            return false;
        }

        const ArchetypeID newArchId = this->findOrCreateArchetype(std::move(newType));
        this->migrate(entity, newArchId);
        for (const ComponentID added_cid: added) {
            this->constructComponent(entity, added_cid);
        }
        for (const ComponentID added_cid: added) {
            this->runOnAdd(entity, added_cid);
        }
        for (const ComponentID added_cid: added) {
            this->addRequiredComponents(entity, added_cid);
        }
        return true;
    }

    void World::constructComponent(const Entity entity, const ComponentID cid) {
        const auto &record = this->component_registry.getRecord(cid);
        if (record.construct != nullptr) {
            record.construct(*this, entity);
        }
    }

    void World::runOnAdd(const Entity entity, const ComponentID cid) {
        const auto &record = this->component_registry.getRecord(cid);
        if (record.onAdd != nullptr) {
            record.onAdd(*this, entity);
        }
    }

    void World::addRequiredComponents(const Entity entity, const ComponentID cid) {
        const auto &record = this->component_registry.getRecord(cid);
        for (const ComponentID required_cid: record.required) {
            this->add_id(entity, required_cid);
        }
    }

    void World::remove_id(const Entity entity, const ComponentID cid) {
        const auto &record = this->entity_registry.getRecord(entity);
        auto &arch = this->archetype_registry.getArchetype(record.archetypeId);
        if (!arch.has(cid))
            return;

        if (cid == reflection::type_id<Name>()) {
            this->clearEntityName(entity);
        }

        if (arch.stores(cid)) {
            const auto &onRemove = arch.columns.get(cid).onRemove;
            for (uint i = 0; i < onRemove.size; i++) {
                onRemove.data[i](arch, record.row);
            }
        }
        ArchetypeID newArchId;
        if (arch.removeEdge.has(cid)) {
            newArchId = arch.removeEdge.get(cid);
        } else {
            EntityType newType = arch.getType().clone();
            newType.remove(cid);
            newArchId =
                    this->findOrCreateArchetype(std::move(newType));
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
        if (!arch.stores(cid)) {
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
        } else {
            for (const ComponentID first = cached._required.data[0]; const ArchetypeID id: this->component_registry.
                 getArchetypes(first)) {
                cached.update(*this, this->archetype_registry.getArchetype(id), id);
            }
        }
    }

    Entity World::entity() {
        return this->entity_registry.create();
    }

    EntityRef World::create() {
        return EntityRef(*this, this->entity_registry.create());
    }

    const char *World::storeEntityName(const std::string &name) {
        this->stored_entity_names.push_back(name);
        return this->stored_entity_names.back().c_str();
    }

    std::string World::makeEntityName(const Entity entity) const {
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
            value = this->makeEntityName(entity);
        }

        Name validated_name(value.c_str());

        if (const auto it = this->entity_name_to_entity.find(value);
            it != this->entity_name_to_entity.end() && !(it->second == entity)) {
            throw std::invalid_argument("duplicate entity name");
        }

        this->clearEntityName(entity);
        name->value = this->storeEntityName(validated_name.value);
        this->entity_name_to_entity[name->value] = entity;
    }

    std::optional<Entity> World::findEntityByName(const std::string &name) const {
        if (const auto it = this->entity_name_to_entity.find(name); it != this->entity_name_to_entity.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void World::kill(const Entity entity) {
        if (const internal::EntityRecord &record = this->entity_registry.getRecord(entity); record.archetypeId != 0) {
            if (this->has<Name>(entity)) {
                this->clearEntityName(entity);
            }
            for (internal::Archetype &arch = this->archetype_registry.getArchetype(record.archetypeId); const auto &sys:
                 arch.onDespawn) {
                sys(arch, record.row);
            }
            this->removeEntityOfArchetype(this->archetype_registry.getArchetype(record.archetypeId), record.row);
        }
        return this->entity_registry.destroy(entity);
    }

    bool World::isAlive(const Entity entity) {
        return this->entity_registry.isAlive(entity);
    }

    void World::runSystem(SystemId sys) {
        auto [phase, index] = sys;
        auto [id, qid, callback, value, run, destroy, condition] = this->phases[phase].systems.at(index);

        if (callback) {
            this->read(qid).iter(callback);
        }
        if (run) {
            run(value, *this);
        }
    }

    QueryID World::cache(Query &&q) {
        const QueryID qid = this->queries.size();
        this->queries.emplace_back(std::move(q));

        this->updateMatches(this->queries.at(qid));

        return qid;
    }

    const datastructures::EcsVec<ArchetypeID> &World::matches(const QueryID qid) const {
        return this->queries.at(qid).matches;
    }

    TablesReader World::read(const QueryID qid) {
        return {this->queries.at(qid).matches, *this};
    }

    const std::vector<SystemRegistered> &World::getSystems(const PhaseId phase) {
        return this->phases[phase].systems;
    }

    void World::runAll(const PhaseId pid) {
        for (auto [id, qid, callback, value, run, destroy, condition]: this->getSystems(pid)) {
            if (condition && !condition(*this)) {
                continue;
            }
            if (callback) {
                this->read(qid).iter(callback);
            }
            if (run) {
                run(value, *this);
            }
        }
    }

    void World::progress() {
        this->runAll(this->phase<PreUpdate>());
        this->runAll(this->phase<Update>());
        this->runAll(this->phase<PostUpdate>());
        this->runAll(this->phase<PreRender>());
        this->runAll(this->phase<Render>());
    }

    void World::start() {
        this->runAll(this->phase<PreStartup>());
        this->runAll(this->phase<Startup>());
        this->runAll(this->phase<PreUpdate>());
    }
} // namespace ecs
