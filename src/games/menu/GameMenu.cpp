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

enum class RequestedScene {
    None,
    Home,
    Selector,
};

namespace {
    constexpr std::string_view LIB_DIR = "./cmake-build-debug/lib"; // Merci clion

    struct SelectableLib {
        std::string path;
        std::string label;
    };

    struct MenuState {
        RequestedScene requestedScene = RequestedScene::None;
        std::optional<std::string> selectedGraphicsLib;
        std::deque<CoreAction> pendingActions;
    };

    MenuState menuState;

    std::string makeDisplayName(const std::filesystem::path& path, const std::string_view prefix) {
        std::string filename = path.filename().string();

        if (filename.starts_with(prefix)) {
            filename.erase(0, prefix.size());
        }
        if (filename.ends_with(".so")) {
            filename.erase(filename.size() - 3);
        }

        return filename;
    }

    std::vector<SelectableLib> scanByPrefix(const std::string_view prefix) {
        std::vector<SelectableLib> libs;
        std::error_code ec;

        if (!std::filesystem::exists(LIB_DIR, ec)) {
            return libs;
        }

        for (std::filesystem::directory_iterator it(LIB_DIR, ec); const auto& entry : it) {
            if (ec || !entry.is_regular_file()) {
                continue;
            }

            const std::filesystem::path& path = entry.path();
            const std::string filename = path.filename().string();

            if (path.extension() != ".so" || !filename.starts_with(prefix)) {
                continue;
            }

            libs.push_back({
                .path = path.string(),
                .label = makeDisplayName(path, prefix)
            });
        }

        std::ranges::sort(libs, [](const SelectableLib& lhs, const SelectableLib& rhs) {
            return lhs.label < rhs.label;
        });
        return libs;
    }

    void queueSceneSwitch(const RequestedScene scene) {
        menuState.requestedScene = scene;
    }

    void queueSelectionActions(const std::string& gamePath) {
        if (menuState.selectedGraphicsLib.has_value()) {
            menuState.pendingActions.push_back({
                .type = CoreActionType::SwitchGraphics,
                .target = *menuState.selectedGraphicsLib
            });
        }

        menuState.pendingActions.push_back({
            .type = CoreActionType::SwitchGame,
            .target = gamePath
        });
    }
}

std::optional<CoreAction> GameMenu::consumeCoreAction() {
    if (menuState.pendingActions.empty()) {
        return std::nullopt;
    }

    CoreAction next = menuState.pendingActions.front();
    menuState.pendingActions.pop_front();
    return next;
}

SYSTEM(MenuSelectorBackgroundSys, On<Update>) {
    static void run(ecs::World& world) {
        world.api->setClearColor({94, 70, 130, 255});
    }
};

SYSTEM(MenuHomeBackgroundSys, On<Update>) {
    static void run(ecs::World& world) {
        world.api->setClearColor({255, 172, 104, 255});
    }
};

void GameMenu::update(IDisplayModule* api) {
    if (menuState.requestedScene == RequestedScene::Selector) {
        this->setScene<SelectorScene>();
        menuState.requestedScene = RequestedScene::None;
    } else if (menuState.requestedScene == RequestedScene::Home) {
        this->setScene<DefaultScene>();
        menuState.requestedScene = RequestedScene::None;
    }
    Engine::update(api);
}

void mainMenuEntities(ecs::World& world) {
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
        Sprite{MENU_BANNER, banner.scale, banner.rect},
        TextOnSpriteComponent(MENU_FONT, "Arcade", Color{40, 24, 16, 255}, 156, {250, 100})
    );

    world.create<Button>({
        .pos = {760, 560},
        .scale = 5.4f,
        .animated = true
    }).listen<ClickedEvent>([](ecs::World&, ecs::Entity, const ClickedEvent&) {
        queueSceneSwitch(RequestedScene::Selector);
    }).set(TextOnSpriteComponent(MENU_FONT, "Play !", Color{40, 24, 16, 255}, 100, {100, 5}));
}

void selectorEntities(ecs::World& world) {
    const std::vector<SelectableLib> graphicsLibs = scanByPrefix("arcade_");
    const std::vector<SelectableLib> gameLibs = scanByPrefix("arcade_game_");

    world.create().set(
        Position{680, 20},
        Size{560, 120},
        Sprite{MENU_BANNER, {5.8f, 3.6f}, {.left = 0, .top = 0, .width = 96, .height = 32}},
        TextOnSpriteComponent(MENU_FONT, "Select Graphics & Game", Color{40, 24, 16, 255}, 52, {34, 25})
    );

    world.create<Button>({
        .pos = {60, 900},
        .scale = 3.8f,
        .animated = true
    }).listen<ClickedEvent>([](ecs::World&, ecs::Entity, const ClickedEvent&) {
        queueSceneSwitch(RequestedScene::Home);
    }).set(TextOnSpriteComponent(MENU_FONT, "Back", Color{40, 24, 16, 255}, 52, {78, 16}));

    constexpr float listScale = 4.2f;
    constexpr float baseY = 180.f;
    constexpr float stepY = 165.f;

    int row = 0;
    for (const auto& lib : graphicsLibs) {
        if (lib.path.find("arcade_game_") != std::string::npos) {
            continue;
        }

        world.create<Button>({
            .pos = {120, baseY + static_cast<float>(row) * stepY},
            .scale = listScale,
            .animated = true
        }).listen<ClickedEvent>([path = lib.path](ecs::World&, ecs::Entity, const ClickedEvent&) {
            menuState.selectedGraphicsLib = path;
        }).set(TextOnSpriteComponent(MENU_FONT, lib.label, Color{40, 24, 16, 255}, 36, {25, 34}));

        row += 1;
    }

    if (row == 0) {
        world.create().set(
            Position{120, baseY},
            Size{380, 50},
            Sprite{MENU_BANNER, {4.f, 1.6f}, {.left = 0, .top = 0, .width = 96, .height = 32}},
            TextOnSpriteComponent(MENU_FONT, "No graphics lib", Color{40, 24, 16, 255}, 28, {20, 9})
        );
    }

    row = 0;
    for (const auto& lib : gameLibs) {
        if (lib.path.find("arcade_game_menu.so") != std::string::npos) {
            continue;
        }

        world.create<Button>({
            .pos = {1060, baseY + static_cast<float>(row) * stepY},
            .scale = listScale,
            .animated = true
        }).listen<ClickedEvent>([path = lib.path](ecs::World&, ecs::Entity, const ClickedEvent&) {
            queueSelectionActions(path);
        }).set(TextOnSpriteComponent(MENU_FONT, lib.label, Color{40, 24, 16, 255}, 36, {25, 34}));

        row += 1;
    }

    if (row == 0) {
        world.create().set(
            Position{1060, baseY},
            Size{380, 50},
            Sprite{MENU_BANNER, {4.f, 1.6f}, {.left = 0, .top = 0, .width = 96, .height = 32}},
            TextOnSpriteComponent(MENU_FONT, "No game lib", Color{40, 24, 16, 255}, 28, {20, 9})
        );
    }
}

GameMenu::GameMenu() :
    Engine("Example", [](Engine& engine, IDisplayModule* api) {
               ecs::World& world = engine.scene<DefaultScene>();
               ecs::World& selectorWorld = engine.scene<SelectorScene>();

               engine.setScene<DefaultScene>();
               world.system<MenuHomeBackgroundSys>();
               selectorWorld.system<MenuSelectorBackgroundSys>();
               api->setClearColor({255, 172, 104, 255});
               api->setWindowSize({1920, 1080});

               mainMenuEntities(world);
               selectorEntities(selectorWorld);
           },
           {
               Resource::texture("./assets/Buttons/Large/UI_Wood_Button_Large_Lock_02a1.png"),
               Resource::texture("./assets/Buttons/Large/UI_Wood_Button_Large_Lock_02a2.png"),
               Resource::texture("./assets/Buttons/Large/UI_Wood_Button_Large_Lock_02a3.png"),
               Resource::font("./assets/Fonts/pixellari.ttf"),
               Resource::texture("./assets/Menu/UI_Wood_Banner_02.png")
           }) {}

extern "C" IGameModule* load() {
    return reinterpret_cast<IGameModule*>(new GameMenu());
}

extern "C" void unload(const IGameModule* game) {
    delete game;
}