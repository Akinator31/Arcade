
#include "engine/ecs/World.hpp"



struct Overlap {
    float x, y;
};

struct PhysicsQuery {
    ecs::QueryID qid;
};

void handle_colision(float origin, float &destination, float dx,
               float otherPos, float combinedHalf) {
    if (dx > 0) {
        float bound = otherPos - combinedHalf;
        if (origin <= bound && destination >= bound)
            destination = bound;
    } else if (dx < 0) {
        float bound = otherPos + combinedHalf;
        if (origin >= bound && destination <= bound)
            destination = bound;
    }
}

void verify_entity(Position &pos, Size &size, float dx, float dy,
                 float &newX, float &newY, ArchetypeView &other,
                 ecs::Entity self) {
    auto *pos_b = other.column<Position>();
    auto *sizes_b = other.optional<Size>();

    for (uint b = 0; b < other.count(); b++) {
        if (self == other.entity(b))
            continue;

        float minDistanceX = (size.width + sizes_b[b].width) * 0.5f;
        float minDistanceY = (size.height + sizes_b[b].height) * 0.5f;

        if (std::abs(pos.y - pos_b[b].y) >= minDistanceY)
            continue;

        handle_colision(pos.x, newX, dx, pos_b[b].x, minDistanceX);

        if (std::abs(newX - pos_b[b].x) >= minDistanceX)
            continue;

        handle_colision(pos.y, newY, dy, pos_b[b].y, minDistanceY);
    }
}

SYSTEM(Move, With<Velocity, Position>, On<PostUpdate>) {
    ITER(view) {
        auto *positions = view.column<Position>();
        auto *velocities = view.column<Velocity>();
        auto *sizes = view.optional<Size>();
        const ecs::QueryID qid = view.world.singleton_get<PhysicsQuery>()->qid;

        for (uint a = 0; a < view.count(); a++) {
            float dx = velocities[a].x * view.world.deltaTime;
            float dy = velocities[a].y * view.world.deltaTime;
            float destinationX = positions[a].x + dx;
            float destinationY = positions[a].y + dy;

            view.world.read(qid).iter([&](ArchetypeView &other) {
                        verify_entity(positions[a], sizes[a], dx, dy,
                                    destinationX, destinationY, other, view.entity(a));
                    });
            positions[a].x = destinationX;
            positions[a].y = destinationY;
        }
    }
};

Overlap verifyColision(Position p1, Position p2,
                       Size s1, Size s2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;

    float overlapX = (s1.width + s2.width) * 0.5f - std::abs(dx);
    float overlapY = (s1.height + s2.height) * 0.5f - std::abs(dy);

    if (overlapX > 0 && overlapY > 0)
        return {overlapX, overlapY};

    return {0,0};
}

void handle_colisions(float x, float y, Position &p1, Position &p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;

    if (x < y)
        p1.x += (dx > 0 ? x : -x);
    else
        p2.y += (dy > 0 ? y : -y);
}

SYSTEM(PhysicsSys, With<Position, Size>, On<PreRender>) {
    ITER(view) {
        auto *positions = view.column<Position>();
        auto *sizes = view.optional<Size>();
        const ecs::QueryID qid = view.world.singleton_get<PhysicsQuery>()->qid;

        for (uint a = 0; a < view.count(); a++) {
            view.world.read(qid).iter([&](ArchetypeView &other) {
                auto *positions_b = other.column<Position>();
                auto *sizes_b = other.optional<Size>();

                for (uint b = 0; b < other.count(); b++) {
                    if (view.entity(a) != other.entity(b)) {
                        auto [x, y] = verifyColision(positions[a], positions_b[b], sizes[a], sizes_b[b]);
                        if (x > 0 || y > 0) {
                            handle_colisions(x, y, positions[a], positions_b[b]);
                        }
                    }
                }
            });
        }
    }
};



struct PhysicsPlugin {

    void load([[maybe_unused]] ecs::World &world) {
        world.registerComponent<Size>();
        world.registerComponent<GlobalPosition>();
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        Query q = query<Position, Size>();
        ecs::QueryID qid = world.cache(std::move(q));

        world.singleton_init<PhysicsQuery>();
        world.singleton_get<PhysicsQuery>()->qid = qid;

        world.system<Move>();
        world.system<PhysicsSys>();
    }

    void unload([[maybe_unused]] ecs::World &world) {
        world.remove<PhysicsSys>();
        world.remove<Move>();
    }
};