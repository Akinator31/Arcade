//
// Created by pavel on 08/03/2026.
//

#pragma once
#include <vector>
#include "type.hpp"
#include "World.hpp"
#include "internal/Archetype.hpp"
#include "engine/reflection/type_id.hpp"

class Query {
public:
    ecs::EntityType _required;
    ecs::EntityType _excluded;


    template<typename... Components>
    void required() {
        (this->_required.add(reflection::type_id<Components>()), ...);
    }

    template<typename... Components>
    void excluded() {
        (this->_excluded.add(reflection::type_id<Components>()), ...);
    }

    void require(const ecs::ComponentID cid) {
        this->_required.add(cid);
    }

    void exclude(const ecs::ComponentID cid) {
        this->_excluded.add(cid);
    }

    [[nodiscard]] bool matchTable(const ecs::internal::Archetype &table) const {
        for (const ecs::ComponentID cid: this->_required) {
            if (!table.has(cid))
                return false;
        }
        for (const ecs::ComponentID cid: this->_excluded) {
            if (table.has(cid))
                return false;
        }
        return true;
    }
};

class QueryCache : public Query {
public:
    void (*on_add)(ecs::World &, ecs::ArchetypeID id) = nullptr;

    datastructures::EcsVec<ecs::ArchetypeID> matches = {};

    explicit QueryCache(Query &&q) : Query(std::move(q)) {
    }

    explicit QueryCache() = default;

    void update(ecs::World &world, const ecs::internal::Archetype &archetype, const ecs::ArchetypeID id) {
        if (this->matchTable(archetype)) {
            if (this->on_add) {
                this->on_add(world, id);
            }
            this->matches.push_back(id);
        }
    }
};

class ArchetypeView {
    ecs::internal::Archetype &archetype;

public:
    ecs::World &world;

    explicit ArchetypeView(ecs::internal::Archetype &archetype, ecs::World &world)
        : archetype(archetype), world(world) {
    }

    [[nodiscard]] uint32_t count() const {
        return this->archetype.count();
    }

    [[nodiscard]] ecs::Entity entity(const std::size_t index) const {
        return this->archetype.getEntities()[index];
    }

    [[nodiscard]] const ecs::Entity *entities() const {
        return this->archetype.getEntities();
    }

    template<typename T>
    T *column() {
        return static_cast<T *>(this->archetype.getColumn(reflection::type_id<T>()));
    }

    template<typename T>
    T *optional() {
        if (const ecs::ComponentID cid = reflection::type_id<T>(); archetype.has(cid)) {
            return static_cast<T *>(this->archetype.getColumn(cid));
        }
        return nullptr;
    }

    template<typename T>
    [[nodiscard]] bool has() const {
        return archetype.has(reflection::type_id<T>());
    }
};


template<typename... Components>
Query query() {
    Query q;
    q.required<Components...>();
    return q;
}
