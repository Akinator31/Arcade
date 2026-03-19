#include "ExampleGame.hpp"
#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"


SYSTEM(Player, ecs::EntityRef, On<PreUpdate>) {
    float direction = 2.f;

    explicit Player(ecs::World &world) : EntityRef(world, Position{100.f, 100.f}, Size{100, 100},
                                                   Color{255, 0, 0, 255}, HoveredSensorComponent {}) {
        listen<ClickedEvent>([this](auto &, auto, auto) {
           set(Color {0, 255, 255, 255});
        });
    }

    RUN() {
        auto *position = get<Position>();

        position->x += direction * 2.f;
        if (position->x < 0.f) {
            position->x = 0.f;
            direction = 1.f;
        }
        if (position->x > 700.f) {
            position->x = 700.f;
            direction = -1.f;
        }
    }
};

struct DefaultScene : DefaultPlugin {
};

extern "C" Engine *load() {
    return new Engine("Example", [](Engine &engine) {
        engine.setScene<DefaultScene>();
        engine.scene<DefaultScene>().system<Player>();
    });
}

extern "C" void unload(const IGame *game) {
    delete game;
}