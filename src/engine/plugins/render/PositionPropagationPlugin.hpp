#pragma once

#include "engine/ecs/World.hpp"

SYSTEM(PositionHierarchyPropagationSys, With<GlobalPosition>, Without<Parent>, On<PreRender>) {
    ITER(view) {
        ecs::World &world = view.world;
        const auto *positions = view.optional<Position>();
        auto *global_positions = view.column<GlobalPosition>();

        for (uint32_t i = 0; i < view.count(); i++) {
            if (positions != nullptr) {
                global_positions[i].x = positions[i].x;
                global_positions[i].y = positions[i].y;
            }

            for (const auto &[parent, child]: world.iterRelated<Hierarchy, true>(view.entity(i))) {
                const auto *parent_position = world.get<GlobalPosition>(parent);
                const auto *child_position = world.get<Position>(child);
                auto *child_global_position = world.get<GlobalPosition>(child);

                child_global_position->x = parent_position->x + child_position->x;
                child_global_position->y = parent_position->y + child_position->y;
            }
        }
    }
};

struct PositionPropagationPlugin {
    void load(ecs::World &world) {
        world.system<PositionHierarchyPropagationSys>();
    }

    void unload(ecs::World &world) {
        world.remove<PositionHierarchyPropagationSys>();
    }
};
