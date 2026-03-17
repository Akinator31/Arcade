#pragma once

#include "DynamicLoader.hpp"
#include "arcade/IGame.hpp"

using LoadGame = Member<IGame *(*)(), "load">;
using UnloadGame = Member<void (*)(IGame *), "unload">;
using GameLoader = DynamicLoader<LoadGame, UnloadGame>;
