#pragma once
#include <optional>

#include "Engine.hpp"
#include "plugins/DefaultPlugins.hpp"

struct DefaultScene : MenuScenePlugin {};

struct SelectorScene : MenuScenePlugin {};

enum Texture {
    WOOD_BUTTON,
    WOOD_BUTTON_HOVER,
    WOOD_BUTTON_CLICK,
    MENU_FONT,
    MENU_BANNER
};

class GameMenu : Engine {
public:
    GameMenu();
    void update(IDisplayModule* api) override;
    std::optional<CoreAction> consumeCoreAction() override;
};