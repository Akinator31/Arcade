#include "World.hpp"

namespace ecs {
    World::World() : component_registry(), archetype_registry(this->component_registry) {
        [[maybe_unused]] auto archeId = this->findOrCreateArchetype({});

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
        for (auto &archetype: this->archetype_registry.archetypes) {
            for (const ComponentID cid: archetype.getType()) {
                if (!archetype.stores(cid)) {
                    continue;
                }
                for (internal::EntityRow row = 0; row < archetype.count(); row += 1) {
                    this->runOnRemove(archetype.getEntities()[row], cid, archetype.getComponent(row, cid));
                }
            }
        }
        this->entity_name_to_entity.clear();
    }
} // namespace ecs
