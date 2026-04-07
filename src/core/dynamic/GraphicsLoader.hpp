#pragma once

#include "DynamicLoader.hpp"
#include "IDisplayModule.hpp"

using Create = Member<IDisplayModule* (*)(), "load">;
using Destroy = Member<void (*)(IDisplayModule *), "unload">;
using GraphicsApiLoader = DynamicLoader<Create, Destroy>;
