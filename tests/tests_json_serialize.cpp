#include <criterion/criterion.h>

#include <cstring>
#include <memory>

#include "engine/parsing/JsonSerializer.hpp"

namespace {
    JsonValue make_string(const char *value) {
        return JsonValue::makeString(::strdup(value));
    }

    std::unique_ptr<JsonValue> make_value(JsonValue value) {
        return std::make_unique<JsonValue>(std::move(value));
    }

    void add_member(const JsonValue &object, const char *key, JsonValue value) {
        object.value.object->push_back(JsonObjectEntry{
            .key = JsonCString(::strdup(key)),
            .value = make_value(std::move(value))
        });
    }

    void add_item(const JsonValue &array, JsonValue value) {
        array.value.array->push_back(std::move(value));
    }
}

Test(json_value_serializer, serialize_root_primitives) {
    const std::string null_json = JsonSerializer::serialize(JsonValue::makeNull());
    const std::string number_json = JsonSerializer::serialize(JsonValue::makeNumber(42.5));
    const std::string boolean_json = JsonSerializer::serialize(JsonValue::makeBoolean(true));

    cr_assert_str_eq(null_json.c_str(), "null");
    cr_assert_str_eq(number_json.c_str(), "42.5");
    cr_assert_str_eq(boolean_json.c_str(), "true");
}

Test(json_value_serializer, serialize_string_with_escapes) {
    const JsonValue value = make_string("line \"one\"\nnext\t\\");

    const std::string json = JsonSerializer::serialize(value);

    cr_assert_str_eq(json.c_str(), "\"line \\\"one\\\"\\nnext\\t\\\\\"");
}

Test(json_value_serializer, serialize_array_value) {
    JsonValue array = JsonValue::makeArray();
    add_item(array, JsonValue::makeNumber(1));
    add_item(array, JsonValue::makeBoolean(false));
    add_item(array, make_string("arcade"));
    add_item(array, JsonValue::makeNull());

    const std::string json = JsonSerializer::serialize(array);

    const auto *expected = "[\n"
            "  1,\n"
            "  false,\n"
            "  \"arcade\",\n"
            "  null\n"
            "]";

    cr_assert_str_eq(json.c_str(), expected, "Expected:\n%s\nGot:\n%s", expected, json.c_str());
}

Test(json_value_serializer, serialize_nested_object_value) {
    JsonValue root = JsonValue::makeObject();
    add_member(root, "name", make_string("arcade"));

    JsonValue config = JsonValue::makeObject();
    add_member(config, "width", JsonValue::makeNumber(800));
    add_member(config, "enabled", JsonValue::makeBoolean(true));
    add_member(root, "config", std::move(config));

    JsonValue items = JsonValue::makeArray();
    add_item(items, JsonValue::makeNumber(1));
    add_item(items, JsonValue::makeBoolean(false));
    add_item(items, make_string("ready"));
    add_member(root, "items", std::move(items));

    const std::string json = JsonSerializer::serialize(root);

    const auto *expected = "{\n"
            "  \"name\": \"arcade\",\n"
            "  \"config\": {\n"
            "    \"width\": 800,\n"
            "    \"enabled\": true\n"
            "  },\n"
            "  \"items\": [\n"
            "    1,\n"
            "    false,\n"
            "    \"ready\"\n"
            "  ]\n"
            "}";

    cr_assert_str_eq(json.c_str(), expected, "Expected:\n%s\nGot:\n%s", expected, json.c_str());
}
