#include "ComponentRegistry.hpp"

#include <algorithm>

namespace ecs::internal {
    std::size_t ComponentRegistry::getSize(const ComponentID cid) const {
        if (!this->isRegistered(cid)) {
            return 0;
        }
        return this->components[cid].size;
    }

    bool ComponentRegistry::isRegistered(const ComponentID cid) const {
        return cid < this->components.size() && this->components[cid].registered;
    }

    void ComponentRegistry::registerComponent(const ComponentID cid, const std::size_t size, StructDef *def) {
        if (this->isRegistered(cid)) {
            return;
        }
        if (cid >= this->components.size()) {
            this->components.resize(cid + 1);
        }
        this->components[cid].registered = true;
        this->components[cid].size = size;
        this->components[cid].def = def;
    }

    ComponentRecord &ComponentRegistry::getRecord(const ComponentID cid) {
        return this->components[cid];
    }

    void ComponentRegistry::addArchetype(const ComponentID cid, const ArchetypeID archId) {
        if (cid >= this->components.size()) {
            this->components.resize(cid + 1);
        }
        this->components[cid].archetypes.push_back(archId);
    }

    void ComponentRegistry::addRequired(const ComponentID cid, const ecs::ComponentID requiredCid) {
        if (cid >= this->components.size()) {
            this->components.resize(cid + 1);
        }
        if (std::find(this->components[cid].required.begin(), this->components[cid].required.end(), requiredCid) ==
            this->components[cid].required.end()) {
            this->components[cid].required.push_back(requiredCid);
        }
    }

    static constexpr std::vector<ArchetypeID> emptyArchetypes;

    const std::vector<ArchetypeID> &ComponentRegistry::getArchetypes(const ComponentID cid) const {
        if (cid >= this->components.size()) {
            return emptyArchetypes;
        }
        return this->components[cid].archetypes;
    }
} // namespace ecs::internal
