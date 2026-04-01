#include "PhysicsPlugin.hpp"
#include <algorithm>
#include <unordered_set>

namespace {
    const Zone kDefaultPhysicsBounds = {{-100000.f, -100000.f}, {200000.f, 200000.f}};

    struct CollisionPair {
        ecs::Entity source{};
        ecs::Entity target{};

        bool operator==(const CollisionPair &other) const noexcept {
            return source == other.source && target == other.target;
        }
    };

    struct Allowed {
        bool x = false;
        bool y = false;
    };

    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair &pair) const noexcept {
            const uint64_t a = (static_cast<uint64_t>(pair.source.index) << 32) | pair.source.generation;
            const uint64_t b = (static_cast<uint64_t>(pair.target.index) << 32) | pair.target.generation;
            return std::hash<uint64_t>{}(a) ^ (std::hash<uint64_t>{}(b) << 1);
        }
    };

    struct CollisionState {
        std::unordered_map<CollisionPair, Allowed, CollisionPairHash> previous;
        std::unordered_map<CollisionPair, Allowed, CollisionPairHash> current;
    };

    bool touches(const Zone &a, const Zone &b) {
        return a.position.x <= b.position.x + b.size.width &&
               a.position.x + a.size.width >= b.position.x &&
               a.position.y <= b.position.y + b.size.height &&
               a.position.y + a.size.height >= b.position.y;
    }

    Zone sweepZone(Zone zone, float dx, float dy) {
        if (dx > 0.f) zone.size.width += dx;
        else if (dx < 0.f) zone.position.x += dx, zone.size.width -= dx;

        if (dy > 0.f) zone.size.height += dy;
        else if (dy < 0.f) zone.position.y += dy, zone.size.height -= dy;

        return zone;
    }

    float clampAxis(const Zone &current, float delta, bool horizontal, ecs::Entity self, const auto &hits) {
        if (delta == 0.f) return 0.f;

        float allowed = delta;

        for (const auto &[entity, zone, flags]: hits) {
            if (entity == self || !(flags.to_ulong() & RIGID)) continue;

            const bool overlapsCrossAxis = horizontal
                                               ? current.position.y < zone.position.y + zone.size.height &&
                                                 current.position.y + current.size.height > zone.position.y
                                               : current.position.x < zone.position.x + zone.size.width &&
                                                 current.position.x + current.size.width > zone.position.x;

            if (!overlapsCrossAxis) continue;

            const float curMin = horizontal ? current.position.x : current.position.y;
            const float curMax = horizontal
                                     ? current.position.x + current.size.width
                                     : current.position.y + current.size.height;
            const float objMin = horizontal ? zone.position.x : zone.position.y;
            const float objMax = horizontal ? zone.position.x + zone.size.width : zone.position.y + zone.size.height;

            if (delta > 0.f) {
                if (objMin < curMax) continue;
                allowed = std::min(allowed, objMin - curMax);
            } else {
                if (objMax > curMin) continue;
                allowed = std::max(allowed, objMax - curMin);
            }
        }

        return allowed;
    }
}

void PhysicsSys::iter(ArchetypeView &view) {
    auto *state = view.world.singleton_get<CollisionState>();
    auto *positions = view.column<Position>();
    const auto *sizes = view.column<Size>();
    auto *velocities = view.column<Velocity>();
    const auto *bodies = view.column<RigidBody>();
    const auto *emitEvents = view.optional<EmitCollisionEvent>();
    auto *query = view.world.singleton_get<SpatialQuery>();
    const bool emits = emitEvents != nullptr;
    const float dt = view.world.deltaTime;

    for (uint i = 0; i < view.count(); ++i) {
        const ecs::Entity self = view.entity(i);
        const Zone body{positions[i], sizes[i]};
        const float dx = velocities[i].x * dt;
        const float dy = velocities[i].y * dt;
        const auto &hits = query->query(sweepZone(body, dx, dy));

        float allowedX = dx;
        float allowedY = dy;

        if (bodies[i] == RIGID) {
            allowedX = clampAxis(body, dx, true, self, hits);
            allowedY = clampAxis({{body.position.x + allowedX, body.position.y}, body.size}, dy, false, self, hits);

            if (allowedX != dx) velocities[i].x = 0.f;
            if (allowedY != dy) velocities[i].y = 0.f;
        }

        positions[i] = {body.position.x + allowedX, body.position.y + allowedY};

        if (!emits) continue;

        const Zone next{positions[i], sizes[i]};
        for (const auto &[entity, zone, flags]: hits) {
            if (entity != self && flags.any() && touches(next, zone)) {
                CollisionPair pair = {self, entity};
                state->current[pair] = Allowed{
                    .x = allowedX == dx, .y = allowedY == dy
                };
            }
        }
    }
}

void IntegrateVelocitySys::iter(ArchetypeView &view) {
    auto *positions = view.column<Position>();
    const auto *velocities = view.column<Velocity>();
    const float dt = view.world.deltaTime;

    for (uint i = 0; i < view.count(); ++i) {
        positions[i].x += velocities[i].x * dt;
        positions[i].y += velocities[i].y * dt;
    }
}

void PhysicsSys::run(ecs::World &world) const {
    auto *state = world.singleton_get<CollisionState>();

    for (const auto &pair: state->current) {
        if (!state->previous.contains(pair.first)) {
            world.emit(pair.first.source, CollisionStart{pair.first.target, pair.second.x, pair.second.y});
        }
    }

    for (const auto &pair: state->previous) {
        if (!state->current.contains(pair.first)) {
            world.emit(pair.first.source, CollisionEnd{pair.first.target});
        }
    }

    state->previous = state->current;
    state->current.clear();
}

void GravitySys::iter(ArchetypeView &view) {
    const auto *gravity = view.column<Gravity>();
    auto *velocities = view.column<Velocity>();
    const float dt = view.world.deltaTime;
    std::cout << dt << std::endl;

    for (uint i = 0; i < view.count(); ++i) {
        std::cout << velocities[i].y << std::endl;
        velocities[i].y += dt * gravity[i].scale;
    }
}

void PhysicsPlugin::load(ecs::World &world) {
    world.plugin<SpatialQueryPlugin>(kDefaultPhysicsBounds);
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
