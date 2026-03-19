#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"

struct DefaultScene;
struct MyPlayer;
struct Enemy;

SYSTEM(Move, With<MyPlayer, Velocity>, On<Update>) {
    ITER(view) {
        auto *velocities = view.column<Velocity>();

        for (uint i = 0; i < view.count(); i++) {
            if (view.world.api->isKeyPressed(KeyboardCode::D)) {
                velocities[i].x = 100;
            } else {
                velocities[i].x = 0;
            }
        }
    }
};

SYSTEM(PlayerSys, ecs::EntityRef, On<Update>) {
    PlayerSys(ecs::World &world) : EntityRef(
        world.create().add<MyPlayer>().set(Position{0, 0}, Size{100, 100}, Velocity{0, 0}, Color::WHITE)) {
        listen<CollisionStart>([](ecs::World &world, Entity entity, CollisionStart event) {
            if (world.has<Enemy>(event.target)) {
                world.kill(entity);
            }
        });
    }

    RUN() {
        if (this->world.api->isKeyPressed(KeyboardCode::Space) && get<Velocity>()->y == 0) {
            get<Velocity>()->y += 100;
        }
    }
};


extern "C" Engine *load() {
    return new Engine("Example", [](Engine &engine) {
        ecs::World &world = engine.scene<DefaultScene>();

        world.create().add<MyPlayer>().set(Position{0, 0}, Size{100, 100}, Gravity{10}, RigidBody::RIGID,
                                           Velocity{300, 0}, Color::WHITE);
        world.create().add<Enemy>().set(Position{300, 0}, Size{100, 100}, Color::BLUE, RigidBody::RIGID);
        world.create().set(Position{-300, 1800}, Size{1000, 100}, Color::RED, RigidBody::RIGID);
    });
}

extern "C" void unload(const IGame *game) {
    delete game;
}
