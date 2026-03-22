#pragma once

#include "DynamicLoader.hpp"
#include "IGameModule.hpp"

using LoadGame = Member<IGameModule *(*)(), "load">;
using UnloadGame = Member<void (*)(IGameModule *), "unload">;
using GameLoader = DynamicLoader<LoadGame, UnloadGame>;
