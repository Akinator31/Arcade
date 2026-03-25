#include "GameMenu.hpp"
#include <algorithm>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>
#include <IDisplayModule.hpp>
#include "engine/Engine.hpp"
#include "IGameModule.hpp"
#include "plugins/render/UiPlugin/UiPrefabs.hpp"

namespace {
    constexpr std::string_view LIB_DIR = "./cmake-build-debug/lib"; // Merci clion

    struct SelectableLib {
        std::string path;
        std::string label;
    };


    std::string makeDisplayName(std::string filename, const std::string_view prefix) {
        filename.erase(0, prefix.size());
        filename.erase(filename.size() - 3);
        return filename;
    }

    std::vector<SelectableLib> scanByPrefix(const std::string_view prefix) {
        std::vector<SelectableLib> libs;
        std::error_code ec;

        if (!std::filesystem::exists(LIB_DIR, ec)) {
            return libs;
        }

        for (std::filesystem::directory_iterator it(LIB_DIR, ec); const auto &entry: it) {
            if (const std::string &path = entry.path().string();
                path.ends_with(".so") && path.starts_with(prefix) && !ec && entry.is_regular_file()) {
                libs.push_back(SelectableLib{
                    .path = path,
                    .label = makeDisplayName(entry.path().filename().string(), prefix)
                });
            }
        }

        std::ranges::sort(libs, [](const SelectableLib &lhs, const SelectableLib &rhs) {
            return lhs.label < rhs.label;
        });
        return libs;
    }
}


SYSTEM(MenuSelectorBackgroundSys, On<Update>) {
    static void run(ecs::World &world) {
        world.api->setClearColor({94, 70, 130, 255});
    }
};

SYSTEM(MenuHomeBackgroundSys, On<Update>) {
    static void run(ecs::World &world) {
        world.api->setClearColor({255, 172, 104, 255});
    }
};

void mainMenuEntities(ecs::World &world) {
    constexpr Sprite banner{
        MENU_BANNER,
        {
            10.f,
            10.f
        },
        {
            .left = 0,
            .top = 0,
            .width = 96,
            .height = 32
        }
    };

    world.create().set(
        Position{460, 0},
        Size{
            static_cast<float>(banner.rect.width) * banner.scale.x,
            static_cast<float>(banner.rect.height) * banner.scale.y
        },
        Sprite{MENU_BANNER, banner.scale, banner.rect}
    ).child().set(Position{250, 100}, Text{MENU_FONT, strdup("Arcade"), Color::red(), 156});

    world.create<Button>({
        .pos = {760, 560},
        .scale = 5.4f,
        .animated = true
    }).listen<ClickedEvent>([](ecs::World &world, ecs::Entity, const ClickedEvent &) {
        world.singleton_get<Engine>()->setScene<SelectorScene>();
    }).child().set(Text{MENU_FONT, strdup("Play !"), Color::red(), 100}, Position{100, 5});
}

void selectorEntities(ecs::World &world) {
    const std::vector<SelectableLib> graphicsLibs = scanByPrefix("arcade_");
    const std::vector<SelectableLib> gameLibs = scanByPrefix("arcade_game_");

    world.create().set(
        Position{680, 20},
        Size{560, 120},
        Sprite{MENU_BANNER, {5.8f, 3.6f}, {.left = 0, .top = 0, .width = 96, .height = 32}}
    ).set(Text{MENU_FONT, strdup("Select Graphics & Game"), Color::red(), 32}, Position{34, 25});

    world.create<Button>({
        .pos = {60, 900},
        .scale = 3.8f,
        .animated = true
    }).listen<ClickedEvent>([](ecs::World &world, ecs::Entity, const ClickedEvent &) {
        world.singleton_get<Engine>()->setScene<DefaultScene>();
    }).child().set(Text(MENU_FONT, strdup("Back"), Color{40, 24, 16, 255}, 52), Position{78, 16});

    constexpr float listScale = 4.2f;
    constexpr float baseY = 180.f;
    constexpr float stepY = 165.f;

    int row = 0;
    for (const auto &[path, label]: graphicsLibs) {
        if (path.find("arcade_game_") != std::string::npos) {
            continue;
        }

        world.create<Button>({
            .pos = {120, baseY + static_cast<float>(row) * stepY},
            .scale = listScale,
            .animated = true
        }).listen<ClickedEvent>([path](ecs::World &world, ecs::Entity, const ClickedEvent &) {
            world.singleton_get<Engine>()->pendingActions.push_back(CoreAction{
                .type = CoreActionType::SwitchGraphics,
                .target = path
            });
        }).child().set(Text(MENU_FONT, strdup(label.c_str()), Color{40, 24, 16, 255}, 36), Position{25, 34});

        row += 1;
    }

    if (row == 0) {
        world.create().set(
            Position{120, baseY},
            Size{380, 50},
            Sprite{MENU_BANNER, {4.f, 1.6f}, {.left = 0, .top = 0, .width = 96, .height = 32}}
        ).child().set(Text(MENU_FONT, strdup("No graphics lib"), Color{40, 24, 16, 255}, 28), Position{20, 9});
    }

    row = 0;
    for (const auto &[path, label]: gameLibs) {
        if (path.find("arcade_game_menu.so") != std::string::npos) {
            continue;
        }

        world.create<Button>({
            .pos = {1060, baseY + static_cast<float>(row) * stepY},
            .scale = listScale,
            .animated = true
        }).listen<ClickedEvent>([path](ecs::World &world, ecs::Entity, const ClickedEvent &) {
            world.singleton_get<Engine>()->pendingActions.push_back(CoreAction{
                .type = CoreActionType::SwitchGame,
                .target = path
            });
        }).child().set(Text{MENU_FONT, strdup(label.c_str()), Color{40, 24, 16, 255}, 36}, Position{25, 34});

        row += 1;
    }

    if (row == 0) {
        world.create().set(
            Position{1060, baseY},
            Size{380, 50},
            Sprite{MENU_BANNER, {4.f, 1.6f}, {.left = 0, .top = 0, .width = 96, .height = 32}}
        ).child().set(Text(MENU_FONT, strdup("No game lib"), Color{40, 24, 16, 255}, 28), Position{20, 9});
    }
}

extern "C" IGameModule *load() {
    auto *engine = new Engine(
        "Example",
        [](Engine &engine, IDisplayModule *api) {
            ecs::World &world = engine.scene<DefaultScene>();
            ecs::World &selectorWorld = engine.scene<SelectorScene>();

            engine.setScene<DefaultScene>();
            world.system<MenuHomeBackgroundSys>();
            selectorWorld.system<MenuSelectorBackgroundSys>();
            api->setClearColor({255, 172, 104, 255});
            api->setWindowSize({1920, 1080});

            mainMenuEntities(world);
            selectorEntities(selectorWorld);
        }, {
            Resource::texture("./assets/Buttons/Large/UI_Wood_Button_Large_Lock_02a1.png"),
            Resource::texture("./assets/Buttons/Large/UI_Wood_Button_Large_Lock_02a2.png"),
            Resource::texture("./assets/Buttons/Large/UI_Wood_Button_Large_Lock_02a3.png"),
            Resource::font("./assets/Fonts/pixellari.ttf"),
            Resource::texture("./assets/Menu/UI_Wood_Banner_02.png")
        });
    return reinterpret_cast<IGameModule *>(engine);
}

extern "C" void unload(const IGameModule *game) {
    delete game;
}
