#pragma once

#include "DynamicLoader.hpp"
#include "IDisplayModule.hpp"

using Create = Member<IDisplayModule* (*)(), "create">;
using Destroy = Member<void (*)(IDisplayModule *), "destroy">;
using GraphicsApiLoader = DynamicLoader<Create, Destroy>;
