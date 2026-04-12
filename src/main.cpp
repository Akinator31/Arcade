#include <iostream>
#include <memory>
#include <exception>
#include "core/dynamic/GameLoader.hpp"
#include "core/dynamic/GraphicsLoader.hpp"
#include "IDisplayModule.hpp"


int main(const int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: arcade <graphics_lib.so>\n";
        return 84;
    }

    try {
        auto graphics_loader = std::make_unique<GraphicsApiLoader>(argv[1]);
        auto game_loader = std::make_unique<GameLoader>("lib/arcade_menu.so");

        IDisplayModule *api = graphics_loader->call<Create>();
        IGameModule *game = game_loader->call<LoadGame>();

        if (!api || !game) {
            std::cerr << "Failed to load graphics API or game module.\n";
            return 84;
        }

        api->init();
        api->loadResources(game->getResources());
        bool shouldQuit = false;

        while (api && api->isWindowOpen() && !shouldQuit) {
            api->beginFrame();
            game->update(api);

            if (api->isKeyPressed(KeyboardCode::M)) {
                game_loader->call<UnloadGame>(game);
                game_loader = std::make_unique<GameLoader>("lib/arcade_menu.so");
                game = game_loader->call<LoadGame>();
                if (game) api->loadResources(game->getResources());
            }

            if (auto action = game->consumeCoreAction(); action.has_value()) {
                switch (action->type) {
                    case CoreActionType::SwitchGame: {
                        game_loader->call<UnloadGame>(game);
                        game_loader = std::make_unique<GameLoader>(action->target);
                        game = game_loader->call<LoadGame>();
                        if (game) api->loadResources(game->getResources());
                        break;
                    }
                    case CoreActionType::SwitchGraphics: {
                        api->shutdown();
                        graphics_loader->call<Destroy>(api);
                        graphics_loader = std::make_unique<GraphicsApiLoader>(action->target);
                        api = graphics_loader->call<Create>();
                        if (api) {
                            api->init();
                            api->loadResources(game->getResources());
                            api->beginFrame();
                        }
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

            if (api) api->endFrame();
        }

        if (api) {
            api->shutdown();
            graphics_loader->call<Destroy>(api);
        }
        if (game) {
            game_loader->call<UnloadGame>(game);
        }
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 84;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception" << std::endl;
        return 84;
    }
    return 0;
}