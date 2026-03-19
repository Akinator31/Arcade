#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"

struct DefaultScene;
struct Enemy;


SYSTEM(PlayerSys, ecs::EntityRef, On<Update>) {
    explicit PlayerSys(ecs::World &world) : EntityRef(
        world.create().set(Position{0, 1500}, Size{100, 100}, Velocity{10, 0}, Color::WHITE(),
                           Gravity{100}, RigidBody::RIGID)) {
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


extern "C" IGameModule *load() {
    return reinterpret_cast<IGameModule *>(new Engine("Example", [](Engine &engine) {
        ecs::World &world = engine.scene<DefaultScene>();
        engine.setScene<DefaultScene>();
        world.plugin<DefaultPlugin>();
        world.registerComponent<Enemy>();
        world.system<PlayerSys>();
        world.create().add<Enemy>().set(Position{300, 1500}, Size{100, 100}, Color::BLUE(), RigidBody::RIGID);
        world.create().set(Position{-300, 1800}, Size{10000, 100}, Color::RED(), RigidBody::RIGID);
    }, {
        Resource::texture("./assets/pacman.png")
    }));
}

extern "C" void unload(const IGameModule *game) {
    delete game;
}
