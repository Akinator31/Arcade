#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"
#include "arcade/IGameModule.hpp"
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
    explicit PlayerSys(ecs::World& world) : EntityRef(
        world.create().set(Position{0, 1500}, Size{100, 100}, Velocity{350, 0}, Color::white(),
                           Gravity{1100}, RigidBody::RIGID, GroundSensorComponent{})) {
        listen<CollisionStart>([](ecs::World& world, Entity entity, CollisionStart event) {
            if (world.has<Enemy>(event.target)) {
                world.kill(entity);
            }
        });

        Sprite sprite = {
            .texture = START_BUTTON,
            .scale = {
                .x = 1,
                .y = 1
            },
            .rect = {
                .left = 0,
                .top = 0,
                .width = 300,
                .height = 151,
            }
        };

        Sprite spriteOnHover = {
            .texture = START_BUTTON_HOVER,
            .scale = {
                .x = 1,
                .y = 1
            },
            .rect = {
                .left = 0,
                .top = 0,
                .width = 297,
                .height = 151,
            }
        };

        //world.set(camera_plugin_impl::mainCamera(world), CameraFollow{this->entity(), {0.f, 0.f}, true});

        world.create<Button>({sprite, spriteOnHover, {300, 300}});
    }

    RUN() {
        if (this->world.api->isKeyPressed(Space) && this->world.has<IsOnGround>(this->entity())) {
            get<Velocity>()->y = -500;
        }
    }
};


extern "C" IGameModule* load() {
    return reinterpret_cast<IGameModule*>(new Engine("Example", [](Engine& engine) {
        ecs::World& world = engine.scene<DefaultScene>();
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

extern "C" void unload(const IGameModule* game) {
    delete game;
}