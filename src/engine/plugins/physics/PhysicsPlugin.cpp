#include "PhysicsPlugin.hpp"

#include <algorithm>
#include <unordered_set>

namespace {
    struct CollisionPair {
        ecs::Entity source{};
        ecs::Entity target{};

        bool operator==(const CollisionPair &other) const noexcept {
            return this->source == other.source && this->target == other.target;
        }
    };

    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair &pair) const noexcept {
            const uint64_t a = (static_cast<uint64_t>(pair.source.index) << 32) | pair.source.generation;
            const uint64_t b = (static_cast<uint64_t>(pair.target.index) << 32) | pair.target.generation;
            return std::hash<uint64_t>{}(a) ^ (std::hash<uint64_t>{}(b) << 1);
        }
    };

    struct CollisionState {
        std::unordered_set<CollisionPair, CollisionPairHash> previous;
        std::unordered_set<CollisionPair, CollisionPairHash> current;
    };

    bool isRigidBody(const std::bitset<16> &flags) {
        return (flags.to_ulong() & RIGID) != 0;
    }

    bool touches(const Zone &a, const Zone &b) {
        return a.position.x <= b.position.x + b.size.width &&
               a.position.x + a.size.width >= b.position.x &&
               a.position.y <= b.position.y + b.size.height &&
               a.position.y + a.size.height >= b.position.y;
    }

    /** @brief Returns the broad phase area covering the whole movement. */
    Zone sweepZone(const Zone &zone, const float dx, const float dy) {
        Zone area = zone;

        if (dx > 0.f) {
            area.size.width += dx;
        } else if (dx < 0.f) {
            area.position.x += dx;
            area.size.width -= dx;
        }

        if (dy > 0.f) {
            area.size.height += dy;
        } else if (dy < 0.f) {
            area.position.y += dy;
            area.size.height -= dy;
        }

        return area;
    }

    /** @brief expands the query box so edge contacts are detected too. */
    Zone contactZone(const Zone &zone) {
        constexpr float margin = 0.001f;
        return {
            .position = {zone.position.x - margin, zone.position.y - margin},
            .size = {zone.size.width + margin * 2.f, zone.size.height + margin * 2.f}
        };
    }

    bool overlapsOnCrossAxis(const Zone &body, const Zone &other, const bool horizontal) {
        if (horizontal) {
            return body.position.y < other.position.y + other.size.height &&
                   body.position.y + body.size.height > other.position.y;
        }
        return body.position.x < other.position.x + other.size.width &&
               body.position.x + body.size.width > other.position.x;
    }

    /** @brief clamp movement on one axis if collid. */
    float resolveAxis(const Zone &body, const float delta, const datastructures::EcsVec<EntityZone> &hits,
                      const ecs::Entity self, const bool horizontal) {
        float allowed = delta;

        for (const auto &[entity, zone, flags]: hits) {
            if (entity == self || !isRigidBody(flags) || !overlapsOnCrossAxis(body, zone, horizontal)) {
                continue;
            }

            if (horizontal && delta > 0.f) {
                allowed = std::min(allowed, zone.position.x - (body.position.x + body.size.width));
            } else if (horizontal && delta < 0.f) {
                allowed = std::max(allowed, zone.position.x + zone.size.width - body.position.x);
            } else if (!horizontal && delta > 0.f) {
                allowed = std::min(allowed, zone.position.y - (body.position.y + body.size.height));
            } else if (!horizontal && delta < 0.f) {
                allowed = std::max(allowed, zone.position.y + zone.size.height - body.position.y);
            }
        }

        return allowed;
    }

    /** @brief resolv a rigide body with a simple X then Y. */
    void resolveBody(ArchetypeView &view, const uint index, CollisionState &state) {
        auto *positions = view.column<Position>();
        const auto *sizes = view.column<Size>();
        auto *velocities = view.column<Velocity>();
        const auto *emit_events = view.optional<EmitCollisionEvent>();
        auto *query = view.world.singleton_get<SpatialQuery>();

        Position &position = positions[index];
        Velocity &velocity = velocities[index];
        const Zone body = {position, sizes[index]};
        const float dx = velocity.x * view.world.deltaTime;
        const float dy = velocity.y * view.world.deltaTime;
        const auto &hits = query->query(sweepZone(body, dx, dy));
        const float allowed_x = resolveAxis(body, dx, hits, view.entity(index), true);
        const Zone moved_x = {{position.x + allowed_x, position.y}, body.size};
        const float allowed_y = resolveAxis(moved_x, dy, hits, view.entity(index), false);
        const Zone next = {{moved_x.position.x, position.y + allowed_y}, body.size};

        position = {next.position.x, next.position.y};

        if (allowed_x != dx) {
            velocity.x = 0.f;
        }
        if (allowed_y != dy) {
            velocity.y = 0.f;
        }

        if (emit_events == nullptr) {
            return;
        }

        for (const auto &[entity, zone, flags]: query->query(contactZone(next))) {
            if (entity != view.entity(index) && isRigidBody(flags) && touches(next, zone)) {
                state.current.insert({view.entity(index), entity});
            }
        }
    }
}

void IntegrateVelocitySys::iter(ArchetypeView &view) {
    auto *positions = view.column<Position>();
    const auto *velocities = view.column<Velocity>();

    for (uint i = 0; i < view.count(); i++) {
        positions[i].x += velocities[i].x * view.world.deltaTime;
        positions[i].y += velocities[i].y * view.world.deltaTime;
    }
}

void PhysicsSys::iter(ArchetypeView &view) {
    auto *state = view.world.singleton_get<CollisionState>();
    if (state == nullptr) {
        return;
    }

    for (uint i = 0; i < view.count(); i++) {
        resolveBody(view, i, *state);
    }
}

void PhysicsSys::run(ecs::World &world) const {
    auto *state = world.singleton_get<CollisionState>();
    if (state == nullptr) {
        return;
    }

    for (const CollisionPair &pair: state->current) {
        if (!state->previous.contains(pair)) {
            world.emit(pair.source, CollisionStart{pair.target});
        }
    }

    for (const CollisionPair &pair: state->previous) {
        if (!state->current.contains(pair)) {
            world.emit(pair.source, CollisionEnd{pair.target});
        }
    }

    state->previous = state->current;
    state->current.clear();
}

void GravitySys::iter(ArchetypeView &view) {
    const auto *gravity = view.column<Gravity>();
    auto *velocities = view.column<Velocity>();
    for (uint i = 0; i < view.count(); i++) {
        velocities[i].y += view.world.deltaTime * gravity->scale;
    }
}

void PhysicsPlugin::load(ecs::World &world) {
    world.plugin<SpatialQueryPlugin>(Zone{{-500, -500}, {3000, 3000}});
    world.registerComponent<Size>();
    world.registerComponent<GlobalPosition>();
    world.registerComponent<Position>();
    world.registerComponent<Velocity>();
    world.registerComponent<RigidBody>();
    world.registerComponent<Gravity>();
    world.registerComponent<EmitCollisionEvent>();
    world.singleton_init<CollisionState>();
    world.system<GravitySys>();
    world.system<PhysicsSys>();
    world.system<IntegrateVelocitySys>();
}

void PhysicsPlugin::unload(ecs::World &world) {
    world.remove<PhysicsSys>();
    world.remove<IntegrateVelocitySys>();
    world.singleton_remove<CollisionState>();
}
