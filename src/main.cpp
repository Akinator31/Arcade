#include <iostream>
#include "core/dynamic/GameLoader.hpp"
#include "core/dynamic/GraphicsLoader.hpp"

int main(const int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: arcade <graphics_lib.so> <game_lib.so>\n";
        return 1;
    }

    GraphicsApiLoader graphics_loader(argv[1]);
    GameLoader game_loader(argv[2]);

    GraphicsApi *api = graphics_loader.call<Create>();
    IGame *game = game_loader.call<LoadGame>();

    api->init();
    while (api->isWindowOpen()) {
        api->beginFrame();
        game->update(api);
        api->endFrame();
    }
    api->shutdown();

    game_loader.call<UnloadGame>(game);
    graphics_loader.call<Destroy>(api);
    return 0;
}
