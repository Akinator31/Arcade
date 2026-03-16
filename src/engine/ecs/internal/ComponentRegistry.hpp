#pragma once

#include "engine/ecs/type.hpp"
#include "engine/reflection/type_id.hpp"
#include <cstddef>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

#include "engine/reflection/rayflect.hpp"

namespace ecs {
    class World;
}

namespace ecs::internal {
    struct ComponentRecord {
        std::size_t size = 0;
        std::vector<ecs::ArchetypeID> archetypes;
        std::vector<ecs::ComponentID> required;

        void (*onAdd)(World &, Entity) = nullptr;

        void (*onRemove)(World &, Entity) = nullptr;

        StructDef *def = nullptr;

        const char *name = nullptr;
    };

    class ComponentRegistry {
    public:
        std::vector<ComponentRecord> components;
        std::unordered_map<std::string, ecs::ComponentID> name_to_id;

        [[nodiscard]] std::size_t getSize(ecs::ComponentID cid) const;

        void registerComponent(ecs::ComponentID cid, std::size_t size, StructDef *def = nullptr);

        void addArchetype(ecs::ComponentID cid, ecs::ArchetypeID archId);

        ComponentRecord &getRecord(ecs::ComponentID cid);

        [[nodiscard]] const std::vector<ecs::ArchetypeID> &
        getArchetypes(ecs::ComponentID cid) const;

        void addRequired(ecs::ComponentID cid, ecs::ComponentID requiredCid);

        template<typename T>
        void registerComponent() {
            if constexpr (HasDef<T>) {
                this->registerComponent(reflection::type_id<T>(), reflection::ecs_sizeof<T>(), T::def());
            } else {
                this->registerComponent(reflection::type_id<T>(), reflection::ecs_sizeof<T>(), nullptr);
            }

            const char *name = type_name<T>();
            this->components[reflection::type_id<T>()].name = name;
            if (name) {
                this->name_to_id[name] = reflection::type_id<T>();
            }
        }
    };
} // namespace ecs::internal
