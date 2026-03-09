//
// Created by pavel on 08/03/2026.
//

#pragma once
#include <vector>
#include "type.hpp"
#include "World.hpp"
#include "internal/Archetype.hpp"
#include "engine/reflection/type_id.hpp"

template<typename... Accessed>
class Query {
public:
    ecs::EntityType _required;
    ecs::EntityType _excluded;

    explicit Query() {
        this->required<Accessed...>();
    }

    explicit Query(ecs::EntityType &&required, ecs::EntityType &&excluded) : _required(std::move(required)),
                                                                             _excluded(std::move(excluded)) {
    }

    template<typename... Components>
    void required() {
        (this->_required.add(reflection::type_id<Components>()), ...);
    }

    template<typename... Components>
    void excluded() {
        (this->_excluded.add(reflection::type_id<Components>()), ...);
    }

    bool matchTable(const ecs::internal::Archetype &table) {
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

    Query<> &&raw() {
        return std::move(*reinterpret_cast<Query<> *>(this));
    }
};

class QueryCache : public Query<> {
public:
    datastructures::EcsVec<ecs::ArchetypeID> matches = {};

    explicit QueryCache(Query<> &&q) : Query<>(std::move(q)) {
    }

    void update(const ecs::internal::Archetype &archetype, const ecs::ArchetypeID id) {
        if (this->matchTable(archetype)) {
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
};


template<typename... Components>
Query<Components...> query() {
    return Query<Components...>();
}
