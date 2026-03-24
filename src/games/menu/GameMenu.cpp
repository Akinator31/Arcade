#include "GameMenu.hpp"

#include <IDisplayModule.hpp>

#include "engine/Engine.hpp"
#include "IGameModule.hpp"
#include "plugins/render/UiPlugin/UiPrefabs.hpp"

CoreAction action{
    .type = CoreActionType::None,
    .target = ""
};

enum class RequestedScene {
    None,
    Home,
    Selector,
};

RequestedScene requestedScene = RequestedScene::None;

std::optional<CoreAction> GameMenu::consumeCoreAction() {
    return {action};
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
    if (requestedScene == RequestedScene::Selector) {
        this->setScene<SelectorScene>();
        requestedScene = RequestedScene::None;
    } else if (requestedScene == RequestedScene::Home) {
        this->setScene<DefaultScene>();
        requestedScene = RequestedScene::None;
    }
    Engine::update(api);
}

GameMenu::GameMenu() :
    Engine("Example", [](Engine& engine, IDisplayModule* api) {
               ecs::World& world = engine.scene<DefaultScene>();
               ecs::World& selectorWorld = engine.scene<SelectorScene>();

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

               constexpr uint32_t titleFontSize = 156;
               constexpr IVec2 textOffset = {250, 100};
               constexpr Size bannerSize = {
                   static_cast<float>(banner.rect.width) * banner.scale.x,
                   static_cast<float>(banner.rect.height) * banner.scale.y
               };

               engine.setScene<DefaultScene>();
               world.system<MenuHomeBackgroundSys>();
               selectorWorld.system<MenuSelectorBackgroundSys>();
               api->setClearColor({255, 172, 104, 255});
               api->setWindowSize({1920, 1080});

               world.create().set(
                   Position{460, 0},
                   bannerSize,
                   Sprite{MENU_BANNER, banner.scale, banner.rect},
                   TextOnSpriteComponent(MENU_FONT, "Arcade", Color{40, 24, 16, 255}, titleFontSize, textOffset)
               );

               world.create<Button>({
                   .pos = {760, 560},
                   .scale = 5.4f,
                   .animated = true
               }).listen<ClickedEvent>([](ecs::World&, ecs::Entity, const ClickedEvent&) {
                   requestedScene = RequestedScene::Selector;
               });
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