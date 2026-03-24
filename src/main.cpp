#include <iostream>
#include "core/dynamic/GameLoader.hpp"
#include "core/dynamic/GraphicsLoader.hpp"
#include "IDisplayModule.hpp"

int main(const int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: arcade <graphics_lib.so>\n";
        return 1;
    }

    GraphicsApiLoader graphics_loader(argv[1]);
    GameLoader game_loader("cmake-build-debug/lib/arcade_game_menu.so");

    IDisplayModule* api = graphics_loader.call<Create>();
    IGameModule* game = game_loader.call<LoadGame>();

    api->init();
    api->loadResources(game->getResources());
    while (api->isWindowOpen()) {
        api->beginFrame();

        game->update(api);

        if (auto action = game->consumeCoreAction(); action.has_value()) {
            switch (action->type) {
            case CoreActionType::SwitchGame: {
                game_loader.call<UnloadGame>(game);
                game = GameLoader(action->target).call<LoadGame>();
                break;
            }
            case CoreActionType::SwitchGraphics: {
                graphics_loader.call<Destroy>(api);
                api = GraphicsApiLoader(action->target).call<Create>();
                api->loadResources(game->getResources());
                break;
            }

            case CoreActionType::Quit: {
                break;
            }

            default:
                break;
            }
        }

        api->endFrame();
    }
    api->shutdown();

    game_loader.call<UnloadGame>(game);
    graphics_loader.call<Destroy>(api);
    return 0;
}