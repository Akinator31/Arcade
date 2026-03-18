#pragma once
#include "type.hpp"
#include "../World.hpp"
#include "engine/parsing/JsonParser.hpp"

struct ComponentJson {
    const char *name;
    JsonValue value;
};

struct EntityJson {
    const char *name;
    std::vector<ComponentJson> components;
};

ecs::Entity entity_from_json(ecs::World &world, const std::string &content) {
    ecs::Entity entity = world.entity();
    return entity;
}