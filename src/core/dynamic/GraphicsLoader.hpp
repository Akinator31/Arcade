#pragma once

#include "DynamicLoader.hpp"
#include "arcade/GraphicsApi.hpp"

using Create = Member<GraphicsApi* (*)(), "create">;
using Destroy = Member<void (*)(GraphicsApi *), "destroy">;
using GraphicsApiLoader = DynamicLoader<Create, Destroy>;
