#include "World.hpp"

namespace ecs {
    void World::runSystem(const SystemId &sys) {
        const auto [phase, index] = sys;
        const SystemRegistered &system = this->phases[phase].systems.at(index);

        if (system.iter != nullptr) {
            this->read(system.qid).iter(system.iter);
        }
        if (system.run != nullptr) {
            system.run(system.value, *this);
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
        for (const SystemRegistered &system: this->getSystems(pid)) {
            if (system.condition != nullptr && !system.condition(*this)) {
                continue;
            }
            if (system.iter != nullptr) {
                this->read(system.qid).iter(system.iter);
            }
            if (system.run != nullptr) {
                system.run(system.value, *this);
            }
            this->flush();
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
