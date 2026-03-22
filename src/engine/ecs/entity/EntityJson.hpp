#pragma once
#include "type.hpp"
#include "../World.hpp"
#include "engine/parsing/JsonParser.hpp"
#include "parsing/JsonSerializer.hpp"

namespace ecs {
    struct ComponentJson {
        const char *name;
        JsonValue value;
    };

    struct EntityJson {
        const char *name;
        std::vector<ComponentJson> components;
    };

    inline std::string entityToJson(World &world, const Entity entity) {
        const auto *name = world.get<Name>(entity);

        const JsonValue object = JsonValue::makeObject();

        auto nameValue = std::make_unique<JsonValue>(
            JsonValue::makeString(strdup(name->value))
        );

        object.value.object->push_back(JsonObjectEntry{
            .key = JsonCString(strdup("name")),
            .value = std::move(nameValue)
        });

        return JsonSerializer::serialize(object);
    }

    inline Entity entityFromJson(World &, const std::string &) {
        return {0, 0};
    }
}
