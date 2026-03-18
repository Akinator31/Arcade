#include "engine/Engine.hpp"
#include "engine/plugins/DefaultPlugins.hpp"

struct DefaultScene : DefaultPlugin {
};

extern "C" Engine *load() {
    return new Engine("Example", [](Engine &engine) {
        engine.setScene<DefaultScene>();
    });
}

extern "C" void unload(const IGame *game) {
    delete game;
}
