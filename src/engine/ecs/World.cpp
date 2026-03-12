#include "World.hpp"

namespace ecs {
    World::World() : component_registry(), archetype_registry(this->component_registry) {
        [[maybe_unused]] auto archeId =
                this->findOrCreateArchetype({});

        this->relation<Hierarchy>();
    }

    World::~World() {
        for (auto &[systems]: this->phases) {
            for (auto &[id, qid, iter, value, run, destroy, condition]: systems) {
                if (destroy) {
                    destroy(value);
                }
            }
        }
        for (auto &[instance, unload, destroy]: this->loaded_plugins) {
            if (instance) {
                unload(instance, *this);
                destroy(instance);
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
        return true;
    }

    void World::remove_id(const Entity entity, const ComponentID cid) {
        const auto &record = this->entity_registry.getRecord(entity);
        auto &arch = this->archetype_registry.getArchetype(record.archetypeId);
        if (!arch.has(cid))
            return;

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

    void World::kill(const Entity entity) {
        if (const internal::EntityRecord &record = this->entity_registry.getRecord(entity); record.archetypeId != 0) {
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

    bool World::add_id_batched(const Entity entity, const ComponentID *cid, const uint32_t count) {
        const auto &record = this->entity_registry.getRecord(entity);
        const auto &arch = this->archetype_registry.getArchetype(record.archetypeId);


        EntityType newType = arch.getType().clone();
        for (uint32_t i = 0; i < count; i++) {
            if (!arch.has(cid[i])) {
                newType.add(cid[i]);
            }
        }

        if (newType.count == arch.getType().count) {
            return false;
        }

        const ArchetypeID newArchId = this->findOrCreateArchetype(std::move(newType));
        this->migrate(entity, newArchId);

        return true;
    }
} // namespace ecs
