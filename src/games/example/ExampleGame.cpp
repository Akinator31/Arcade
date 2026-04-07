#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"
#include "IGameModule.hpp"
#include "plugins/render/UiPlugin/UiPrefabs.hpp"

struct DefaultScene;
struct Enemy;

LibType LIB_TYPE = GAME;

enum class Texture {
    PACMAN,
    START_BUTTON,
    START_BUTTON_HOVER
};

SYSTEM(PlayerSys, ecs::EntityRef, On<Add>) {
    explicit PlayerSys(ecs::World &world) : EntityRef(
        world.create().set(Position{0, 1500}, Size{100, 100}, Velocity{350, 0}, Color::white(),
                           Gravity{1100}, RigidBody::RIGID, GroundSensorComponent{})) {
        listen<CollisionStart>([](ecs::World &world, Entity entity, CollisionStart event) {
            if (world.has<Enemy>(event.target)) {
                world.kill(entity);
            }
        });
    }

    RUN() {
        if (this->world.api->isKeyPressed(Space) && this->world.has<IsOnGround>(this->entity())) {
            get<Velocity>()->y = -500;
        }
    }
};

extern "C" IGameModule *load() {
    return reinterpret_cast<IGameModule *>(new Engine(
        "Example", [](Engine &engine, [[maybe_unused]] IDisplayModule *api) {
            ecs::World &world = engine.scene<DefaultScene>();
            engine.setScene<DefaultScene>();
            world.plugin<DefaultPlugin>();
            world.registerComponent<Enemy>();
            world.system<PlayerSys>();
            world.create().add<Enemy>().set(Position{300, 1500}, Size{100, 100}, Color::blue(), RigidBody::RIGID);
            world.create().set(Position{-300, 1800}, Size{10000, 100}, Color::red(), RigidBody::RIGID);
        }, {
            Resource::texture("./assets/pacman.png"),
            Resource::texture("./assets/Start.png"),
            Resource::texture("./assets/StartHover.png")
        }));
}

extern "C" void unload(const IGameModule *game) {
    delete game;
}
