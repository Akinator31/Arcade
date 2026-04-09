#include <iostream>
#include <memory>
#include "core/dynamic/GameLoader.hpp"
#include "core/dynamic/GraphicsLoader.hpp"
#include "IDisplayModule.hpp"


int main(const int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: arcade <graphics_lib.so>\n";
        return 1;
    }

    auto graphics_loader = std::make_unique<GraphicsApiLoader>(argv[1]);
    auto game_loader = std::make_unique<GameLoader>("lib/arcade_game_menu.so");

    IDisplayModule *api = graphics_loader->call<Create>();
    IGameModule *game = game_loader->call<LoadGame>();

    api->init();
    api->loadResources(game->getResources());
    bool shouldQuit = false;

    while (api->isWindowOpen() && !shouldQuit) {
        api->beginFrame();

        game->update(api);

        if (auto action = game->consumeCoreAction(); action.has_value()) {
            switch (action->type) {
                case CoreActionType::SwitchGame: {
                    game_loader->call<UnloadGame>(game);
                    game_loader = std::make_unique<GameLoader>(action->target);
                    game = game_loader->call<LoadGame>();
                    api->loadResources(game->getResources());
                    break;
                }
                case CoreActionType::SwitchGraphics: {
                    api->shutdown();
                    graphics_loader->call<Destroy>(api);
                    graphics_loader = std::make_unique<GraphicsApiLoader>(action->target);
                    api = graphics_loader->call<Create>();
                    api->init();
                    api->loadResources(game->getResources());
                    break;
                }

                case CoreActionType::Quit: {
                    shouldQuit = true;
                    break;
                }

                default:
                    break;
            }
        }

        api->endFrame();
    }
    api->shutdown();

    game_loader->call<UnloadGame>(game);
    graphics_loader->call<Destroy>(api);
    return 0;
}
