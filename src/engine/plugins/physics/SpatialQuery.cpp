#include "engine/plugins/physics/SpatialQuery.hpp"

#include "PhysicsPlugin.hpp"

bool contains(const Zone &zone, const Position &position) {
    return (
        position.x >= zone.position.x &&
        position.x < zone.position.x + zone.size.width &&
        position.y >= zone.position.y &&
        position.y < zone.position.y + zone.size.height
    );
}

QuadTreeNode::QuadTreeNode() : depth(0), zone{{0, 0}, {0, 0}}, first_child(invalid_child()) {
}

QuadTreeNode::QuadTreeNode(const uint16_t depth, const Zone &zone)
    : depth(depth), zone(zone), first_child(invalid_child()) {
}

bool QuadTreeNode::isLeaf() const {
    return this->first_child == invalid_child();
}

void QuadTreeNode::reset(const uint16_t newDepth, const Zone &newZone) {
    this->depth = newDepth;
    this->zone = newZone;
    this->first_child = invalid_child();
    this->entities.size = 0;
}

QuadTree::QuadTree(const Zone &zone) {
    this->nodes.reserve(256);
    this->nodes.emplace_back(0, zone);
    this->active_nodes = 1;
}

void QuadTree::reset(const Zone &zone) {
    if (this->nodes.empty()) {
        this->nodes.emplace_back(0, zone);
    }
    this->active_nodes = 1;
    this->nodes[0].reset(0, zone);
    this->entities_for_query.size = 0;
    this->entity_ids_for_query.size = 0;
}

const QuadTreeNode &QuadTree::root() const {
    return this->nodes.front();
}

QuadTreeNode &QuadTree::root() {
    return this->nodes.front();
}

uint32_t QuadTree::nodeCount() const {
    return this->active_nodes;
}

bool QuadTree::insert(const EntityZone &entity) {
    if (!overlaps(entity.zone, this->root().zone)) {
        return false;
    }
    this->insertInto(0, entity);
    return true;
}

const datastructures::EcsVec<EntityZone> &QuadTree::query(const Zone &area) {
    this->entities_for_query.size = 0;
    this->forEachInArea(area, [this](const EntityZone &entity) {
        this->entities_for_query.push_back(entity);
    });
    return this->entities_for_query;
}

const datastructures::EcsVec<ecs::Entity> &QuadTree::queryEntities(const Zone &area) {
    this->entity_ids_for_query.size = 0;
    this->forEachEntityInArea(area, [this](const ecs::Entity entity) {
        this->entity_ids_for_query.push_back(entity);
    });
    return this->entity_ids_for_query;
}

const datastructures::EcsVec<EntityZone> &QuadTree::queryPoint(const Position &position) {
    this->entities_for_query.size = 0;
    this->forEachAtPoint(position, [this](const EntityZone &entity) {
        this->entities_for_query.push_back(entity);
    });
    return this->entities_for_query;
}

const datastructures::EcsVec<ecs::Entity> &QuadTree::queryEntitiesAtPoint(const Position &position) {
    this->entity_ids_for_query.size = 0;
    this->forEachEntityAtPoint(position, [this](const ecs::Entity entity) {
        this->entity_ids_for_query.push_back(entity);
    });
    return this->entity_ids_for_query;
}

Zone QuadTree::childZone(const Zone &parent, const uint8_t slot) {
    const float half_width = parent.size.width * 0.5f;
    const float half_height = parent.size.height * 0.5f;
    const bool right = (slot & 1U) != 0;
    const bool bottom = (slot & 2U) != 0;

    return {
        .position = {
            parent.position.x + (right ? half_width : 0.0f),
            parent.position.y + (bottom ? half_height : 0.0f)
        },
        .size = {half_width, half_height}
    };
}

uint8_t QuadTree::childSlotFor(const Zone &zone, const Zone &parent) {
    const float mid_x = parent.position.x + parent.size.width * 0.5f;
    const float mid_y = parent.position.y + parent.size.height * 0.5f;
    const float max_x = zone.position.x + zone.size.width;
    const float max_y = zone.position.y + zone.size.height;

    const bool fits_left = zone.position.x >= parent.position.x && max_x <= mid_x;
    const bool fits_right = zone.position.x >= mid_x && max_x <= parent.position.x + parent.size.width;
    const bool fits_top = zone.position.y >= parent.position.y && max_y <= mid_y;
    const bool fits_bottom = zone.position.y >= mid_y && max_y <= parent.position.y + parent.size.height;

    if ((!fits_left && !fits_right) || (!fits_top && !fits_bottom)) {
        return 4;
    }

    return static_cast<uint8_t>((fits_bottom ? 2U : 0U) | (fits_right ? 1U : 0U));
}

QuadTreeNode &QuadTree::allocateNode(const uint16_t depth, const Zone &zone) {
    const uint32_t index = this->active_nodes;
    if (index >= this->nodes.size()) {
        this->nodes.emplace_back(depth, zone);
    } else {
        this->nodes[index].reset(depth, zone);
    }
    this->active_nodes += 1;
    return this->nodes[index];
}

void QuadTree::split(const uint32_t node_index) {
    const Zone zone = this->nodes[node_index].zone;
    const uint16_t depth = this->nodes[node_index].depth;

    if (!this->nodes[node_index].isLeaf()) {
        return;
    }

    const uint32_t first_child = this->active_nodes;
    this->nodes[node_index].first_child = first_child;

    for (uint8_t slot = 0; slot < 4; ++slot) {
        this->allocateNode(static_cast<uint16_t>(depth + 1), childZone(zone, slot));
    }

    uint32_t entity_index = 0;
    while (entity_index < this->nodes[node_index].entities.size) {
        const EntityZone entity = this->nodes[node_index].entities[entity_index];
        const uint8_t child_slot = childSlotFor(entity.zone, zone);

        if (child_slot >= 4) {
            entity_index += 1;
            continue;
        }

        this->nodes[first_child + child_slot].entities.push_back(entity);
        this->nodes[node_index].entities.erase(entity_index);
    }
}

void QuadTree::insertInto(const uint32_t node_index, const EntityZone &entity) {
    QuadTreeNode &node = this->nodes[node_index];

    if (!node.isLeaf()) {
        if (const uint8_t child_slot = childSlotFor(entity.zone, node.zone); child_slot < 4) {
            this->insertInto(node.first_child + child_slot, entity);
            return;
        }
        node.entities.push_back(entity);
        return;
    }

    if (node.depth >= DEPTH_MAX || node.entities.size < ENTITY_MAX_PER_CELL) {
        node.entities.push_back(entity);
        return;
    }

    this->split(node_index);
    this->insertInto(node_index, entity);
}

SpatialQuery::SpatialQuery(const Zone &bounds) : bounds(bounds), quadtree(bounds) {
}

void SpatialQuery::reset() {
    this->quadtree.reset(this->bounds);
}

bool SpatialQuery::insert(const EntityZone &entity) {
    return this->quadtree.insert(entity);
}

bool SpatialQuery::insert(const ecs::Entity entity, const Zone &zone, const std::bitset<16> flags) {
    return this->insert({.entity = entity, .zone = zone, .flags = flags});
}

const datastructures::EcsVec<EntityZone> &SpatialQuery::query(const Zone &area) {
    return this->quadtree.query(area);
}

const datastructures::EcsVec<ecs::Entity> &SpatialQuery::queryEntities(const Zone &area) {
    return this->quadtree.queryEntities(area);
}

const datastructures::EcsVec<EntityZone> &SpatialQuery::queryPoint(const Position &position) {
    return this->quadtree.queryPoint(position);
}

const datastructures::EcsVec<ecs::Entity> &SpatialQuery::queryEntitiesAtPoint(const Position &position) {
    return this->quadtree.queryEntitiesAtPoint(position);
}

SpatialQueryRebuildSystem::SpatialQueryRebuildSystem(ecs::World &world) {
    Query query;
    query.required<Position, Size>();
    this->query_id = world.cache(std::move(query));
}

void SpatialQueryRebuildSystem::run(ecs::World &world) const {
    auto *spatial_query = world.singleton_get<SpatialQuery>();
    if (spatial_query == nullptr) {
        return;
    }

    spatial_query->reset();
    world.read(this->query_id).iter([spatial_query](ArchetypeView &view) {
        const ecs::Entity *entities = view.entities();
        const Position *positions = view.column<Position>();
        const Size *sizes = view.column<Size>();
        const RigidBody *bodies = view.optional<RigidBody>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            (void) spatial_query->insert(entities[i], {
                                             .position = {positions[i].x, positions[i].y},
                                             .size = sizes[i],
                                         }, bodies ? bodies[i] : 0);
        }
    });
}

SpatialQueryPlugin::SpatialQueryPlugin(const Zone &bounds) : bounds(bounds) {
}

void SpatialQueryPlugin::load(ecs::World &world) const {
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.registerComponent<Size>();
    world.singleton_init(new SpatialQuery(this->bounds));
    world.system<SpatialQueryRebuildSystem>();
}

void SpatialQueryPlugin::unload(ecs::World &world) {
    world.remove<SpatialQueryRebuildSystem>();
    world.singleton_remove<SpatialQuery>();
}
