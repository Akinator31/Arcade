#include <iostream>

#include "engine/Graphics.hpp"
#include "engine/ecs/World.hpp"


struct NoIntegrate;


enum class GameState {
    Menu,
    Game
};

template<typename From, typename To>
SYSTEM(CopyAll, With<From, To>) {
    static_assert(sizeof(From) == sizeof(To), "components size to copy is not eq");
    ITER(view) {
        const From *from = view.column<From>();
        To *to = view.column<To>();

        memcpy(to, from, sizeof(From) * view.count());
    }
};

SYSTEM(PropagatePosition, With<Position>, Without<Parent>, On<PreRender>) {
    ITER(view) {
        ecs::World &world = view.world;
        const auto *positions = view.column<Position>();
        auto *global_positions = view.column<GlobalPosition>();
        const bool has_children = view.has<Children>();

        for (uint i = 0; i < view.count(); i++) {
            global_positions[i].x += positions[i].x;
            global_positions[i].y += positions[i].y;

            if (has_children) {
                for (auto [parent, child]: world.iterRelated<Hierarchy>(view.entity(i))) {
                    const auto *parent_position = world.get<GlobalPosition>(parent);
                    const auto *child_pos = world.get<Position>(child);
                    auto *global_position = world.get<GlobalPosition>(child);

                    global_position->x += child_pos->x + parent_position->x;
                    global_position->y += child_pos->y + parent_position->y;
                }
            }
        }
    }
};


int main() {
    ecs::World world;
}
