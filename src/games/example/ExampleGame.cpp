#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"
#include "IGameModule.hpp"
#include "plugins/render/UiPlugin/UiPrefabs.hpp"

struct DefaultScene;
struct Enemy;

LibType LIB_TYPE = GAME;

enum Texture {
    PACMAN,
    START_BUTTON,
    START_BUTTON_HOVER
};


SYSTEM(PlayerSys, ecs::EntityRef, On<Update>) {
    explicit PlayerSys(ecs::World &world) : EntityRef(world.create()) {
        this->add<EmitCollisionEvent, GroundSensorComponent>().set(Position{100, 100}, Size{100, 100}, Color::blue(), RigidBody::RIGID, Gravity(1600), Velocity(300, 0)).listen<CollisionStart>([this](ecs::World &world, ecs::Entity , CollisionStart collision) {
                if (world.has<Enemy>(collision.target)) {
                    this->set(Position(100, 100), Velocity(300, 0));
                }
        });
        world.set(camera_plugin_impl::mainCamera(world), CameraFollow(this->entity()));
    }
    RUN() {
        if (world.api->isKeyPressed(KeyboardCode::Space) && this->has<IsOnGround>()) {
            this->set(Velocity(300, -500));
        }
    }
};


extern "C" IGameModule* load() {
    return reinterpret_cast<IGameModule*>(new Engine(
        "Example", [](Engine& engine, [[maybe_unused]] IDisplayModule* api) {
            ecs::World& world = engine.scene<DefaultScene>();
            engine.setScene<DefaultScene>();
            world.plugin<DefaultPlugin>();
            world.registerComponent<Enemy>();
            world.create().set(Position{-300, 500}, Size{10000, 100}, Color::red(), RigidBody::RIGID);
            world.create().set(Position(1000, 480), Size(100, 100), RigidBody::RIGID, Color::yellow()).add<Enemy>();
            world.system<PlayerSys>();
        }, {
            Resource::texture("./assets/pacman.png"),
            Resource::texture("./assets/Start.png"),
            Resource::texture("./assets/StartHover.png")
        }));
}

extern "C" void unload(const IGameModule* game) {
    delete game;
}