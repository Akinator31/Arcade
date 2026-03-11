#include <iostream>

#include "engine/Graphics.hpp"
#include "engine/ecs/World.hpp"
#include "engine/plugins/CliPlugin.hpp"
#include "engine/reflection/rayflect.hpp"


struct NoIntegrate;


enum class GameState {
    Menu,
    Game
};

//
// SYSTEM(PropagatePosition, With<Position>, Without<Parent>, On<PreRender>) {
//     ITER(view) {
//         ecs::World &world = view.world;
//         const auto *positions = view.column<Position>();
//         auto *global_positions = view.column<GlobalPosition>();
//         const bool has_children = view.has<Children>();
//
//         for (uint i = 0; i < view.count(); i++) {
//             global_positions[i].x += positions[i].x;
//             global_positions[i].y += positions[i].y;
//
//             if (has_children) {
//                 for (auto [parent, child]: world.iterRelated<Hierarchy>(view.entity(i))) {
//                     const auto *parent_position = world.get<GlobalPosition>(parent);
//                     const auto *child_pos = world.get<Position>(child);
//                     auto *global_position = world.get<GlobalPosition>(child);
//
//                     global_position->x += child_pos->x + parent_position->x;
//                     global_position->y += child_pos->y + parent_position->y;
//                 }
//             }
//         }
//     }
// };

struct Player;
struct Enemy;

[[noreturn]] int main() {
    ecs::World world;

    world.create().add<Position, Enemy>().set(Name{"enemy"});
    world.create().add<Player>().set(Name{"player"}, Position{10., 1.});
    world.create().add<Position, Player>().set(Name{"sasa"});


    world.plugin<CliPlugin>(CliMode::Server, 4040);


    while (true) {
        world.progress();
    }
}
