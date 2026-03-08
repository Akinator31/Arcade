//
// Created by pavel on 08/03/2026.
//

#pragma once
#include <vector>

#include "type.hpp"
#include "World.hpp"
#include "engine/reflection/type_id.hpp"

template <typename... Accessed>
class Query {
    std::vector<ecs::ComponentID> _required;
    std::vector<ecs::ComponentID> _excluded;
    std::vector<ecs::ArchetypeID> _matches;
    ecs::World& _world;

public:
    explicit Query(ecs::World& world) : _world(world) {
        this->required<Accessed...>();
    }

    template <typename... Components>
    void required() {
        (this->_required.push_back(reflection::type_id<Components>()), ...);
    }

    template <typename... Components>
    void excluded() {
        (this->_excluded.push_back(reflection::type_id<Components>()), ...);
    }

    bool matchType(const ecs::EntityType& type) {
        for (const ecs::ComponentID cid : this->_required) {
            if (!type.has(cid))
                return false;
        }
        for (const ecs::ComponentID cid : this->_excluded) {
            if (type.has(cid))
                return false;
        }
        return true;
    }

    void connect() {
        int i = 0;

        for (const ecs::internal::Archetype& table : this->_world.getArchetypes()) {
            if (this->matchType(table.getType())) {
                this->_matches.push_back(i);
            }
            i++;
        }
    }

    uint32_t count() {
        uint32_t result = 0;

        for (const ecs::ArchetypeID tId : this->_matches) {
            const ecs::internal::Archetype& table = this->_world.getArchetypes().at(tId);

            result += table.count();
        }
        return result;
    }
};

template <typename... Components>
Query<Components...> query(ecs::World& world) {
    return Query<Components...>(world);
}