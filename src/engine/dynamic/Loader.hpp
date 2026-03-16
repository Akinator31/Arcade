#pragma once
#include "DynamicLoader.hpp"
#include "../Graphics.hpp"

using Create = Member<GraphicsApi* (*)(), "create">;
using Destroy = Member<void (*)(GraphicsApi *), "destroy">;
using GraphicsApiLoader = DynamicLoader<Create>;
