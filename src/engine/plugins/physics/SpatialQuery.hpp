#pragma once

#include <bitset>
#include <cstdint>
#include <vector>

#include "engine/datastructures/EcsVec.hpp"
#include "engine/ecs/World.hpp"

struct Zone {
    Position position;
    Size size;
};

inline bool overlaps(const Zone &a, const Zone &b) {
    return (
        a.position.x < b.position.x + b.size.width &&
        a.position.x + a.size.width > b.position.x &&
        a.position.y < b.position.y + b.size.height &&
        a.position.y + a.size.height > b.position.y
    );
}

bool contains(const Zone &zone, const Position &position);

inline constexpr uint32_t ENTITY_MAX_PER_CELL = 7;
inline constexpr uint16_t DEPTH_MAX = 8;

struct EntityZone {
    ecs::Entity entity{};
    Zone zone = {{0, 0}, {}};
    std::bitset<16> flags = {};
};

struct QuadTreeNode {
    uint16_t depth;
    Zone zone;
    uint32_t first_child;
    datastructures::EcsVec<EntityZone> entities;

    QuadTreeNode();

    QuadTreeNode(uint16_t depth, const Zone &zone);

    [[nodiscard]] bool isLeaf() const;

    void reset(uint16_t newDepth, const Zone &newZone);

    [[nodiscard]] static constexpr uint32_t invalid_child() {
        return UINT32_MAX;
    }
};

struct QuadTree {
    std::vector<QuadTreeNode> nodes;
    uint32_t active_nodes = 0;
    datastructures::EcsVec<EntityZone> entities_for_query;
    datastructures::EcsVec<ecs::Entity> entity_ids_for_query;

    explicit QuadTree(const Zone &zone);

    void reset(const Zone &zone);

    [[nodiscard]] const QuadTreeNode &root() const;

    [[nodiscard]] QuadTreeNode &root();

    [[nodiscard]] uint32_t nodeCount() const;

    [[nodiscard]] bool insert(const EntityZone &entity);

    template<typename Visitor>
    void forEachInArea(const Zone &area, Visitor &&visitor) const {
        this->visitArea(0, area, visitor);
    }

    template<typename Visitor>
    void forEachEntityInArea(const Zone &area, Visitor &&visitor) const {
        this->forEachInArea(area, [&visitor](const EntityZone &entity) {
            visitor(entity.entity);
        });
    }

    template<typename Visitor>
    void forEachAtPoint(const Position &position, Visitor &&visitor) const {
        this->visitPoint(0, position, visitor);
    }

    template<typename Visitor>
    void forEachEntityAtPoint(const Position &position, Visitor &&visitor) const {
        this->forEachAtPoint(position, [&visitor](const EntityZone &entity) {
            visitor(entity.entity);
        });
    }

    [[nodiscard]] const datastructures::EcsVec<EntityZone> &query(const Zone &area);

    [[nodiscard]] const datastructures::EcsVec<ecs::Entity> &queryEntities(const Zone &area);

    [[nodiscard]] const datastructures::EcsVec<EntityZone> &queryPoint(const Position &position);

    [[nodiscard]] const datastructures::EcsVec<ecs::Entity> &queryEntitiesAtPoint(const Position &position);

private:
    static Zone childZone(const Zone &parent, uint8_t slot);

    static uint8_t childSlotFor(const Zone &zone, const Zone &parent);

    QuadTreeNode &allocateNode(uint16_t depth, const Zone &zone);

    void split(uint32_t node_index);

    void insertInto(uint32_t node_index, const EntityZone &entity);

    template<typename Visitor>
    void visitArea(const uint32_t node_index, const Zone &area, Visitor &visitor) const {
        if (node_index >= this->active_nodes) {
            return;
        }

        const QuadTreeNode &node = this->nodes[node_index];
        if (!overlaps(node.zone, area)) {
            return;
        }

        for (const EntityZone &entity: node.entities) {
            visitor(entity);
        }

        if (node.isLeaf()) {
            return;
        }

        for (uint32_t offset = 0; offset < 4; ++offset) {
            this->visitArea(node.first_child + offset, area, visitor);
        }
    }

    template<typename Visitor>
    void visitPoint(const uint32_t node_index, const Position &position, Visitor &visitor) const {
        if (node_index >= this->active_nodes) {
            return;
        }

        const QuadTreeNode &node = this->nodes[node_index];
        if (!contains(node.zone, position)) {
            return;
        }

        for (const EntityZone &entity: node.entities) {
            if (contains(entity.zone, position)) {
                visitor(entity);
            }
        }

        if (node.isLeaf()) {
            return;
        }

        for (uint32_t offset = 0; offset < 4; ++offset) {
            this->visitPoint(node.first_child + offset, position, visitor);
        }
    }
};

struct SpatialQuery {
    Zone bounds;
    QuadTree quadtree;

    explicit SpatialQuery(const Zone &bounds);

    void reset();

    [[nodiscard]] bool insert(const EntityZone &entity);

    [[nodiscard]] bool insert(ecs::Entity entity, const Zone &zone, std::bitset<16> flags);

    template<typename Visitor>
    void forEachInArea(const Zone &area, Visitor &&visitor) const {
        this->quadtree.forEachInArea(area, visitor);
    }

    template<typename Visitor>
    void forEachEntityInArea(const Zone &area, Visitor &&visitor) const {
        this->quadtree.forEachEntityInArea(area, visitor);
    }

    template<typename Visitor>
    void forEachAtPoint(const Position &position, Visitor &&visitor) const {
        this->quadtree.forEachAtPoint(position, visitor);
    }

    template<typename Visitor>
    void forEachEntityAtPoint(const Position &position, Visitor &&visitor) const {
        this->quadtree.forEachEntityAtPoint(position, visitor);
    }

    [[nodiscard]] const datastructures::EcsVec<EntityZone> &query(const Zone &area);

    [[nodiscard]] const datastructures::EcsVec<ecs::Entity> &queryEntities(const Zone &area);

    [[nodiscard]] const datastructures::EcsVec<EntityZone> &queryPoint(const Position &position);

    [[nodiscard]] const datastructures::EcsVec<ecs::Entity> &queryEntitiesAtPoint(const Position &position);
};

SYSTEM(SpatialQueryRebuildSystem, On<PostUpdate>) {
    ecs::QueryID query_id;

    explicit SpatialQueryRebuildSystem(ecs::World &world);

    RUN(world) const;
};

struct SpatialQueryPlugin {
    Zone bounds;

    explicit SpatialQueryPlugin(const Zone &bounds);

    void load(ecs::World &world) const;

    static void unload(ecs::World &world);
};
