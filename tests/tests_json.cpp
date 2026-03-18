#include <criterion/criterion.h>
#include "engine/parsing/JsonParser.hpp"
#include "engine/parsing/JsonSerializer.hpp"
#include "engine/reflection/Rayflect.hpp"

struct TestStruct {
    float x;
    int32_t y;
    uint32_t z;

    rayflect(TestStruct,
             TestStruct->member<float>("x");
             TestStruct->member<int32_t>("y");
             TestStruct->member<uint32_t>("z");
    )
};

struct TestStructStr {
    const char *name;

    rayflect(TestStructStr,
             TestStructStr->member<const char*>("name");
    )
};

struct TestStructSmallUnsigned {
    uint8_t flags;
    uint16_t id;

    rayflect(TestStructSmallUnsigned,
             TestStructSmallUnsigned->member<uint8_t>("flags");
             TestStructSmallUnsigned->member<uint16_t>("id");
    )
};

Test(json_serializer, serialize_struct) {
    constexpr TestStruct t{1.5f, -42, 100};

    const std::string json = JsonSerializer::serialize(*TestStruct::def(), &t);

    const auto expected = "{\n"
            "  \"x\": 1.5,\n"
            "  \"y\": -42,\n"
            "  \"z\": 100\n"
            "}";

    cr_assert_eq(json, expected, "Expected:\n%s\nGot:\n%s", expected, json.c_str());
}

Test(json_serializer, serialize_string) {
    constexpr TestStructStr t{"hello world"};

    const std::string json = JsonSerializer::serialize(*TestStructStr::def(), &t);

    const auto *expected = "{\n"
            "  \"name\": \"hello world\"\n"
            "}";

    cr_assert_eq(json, expected, "Expected:\n%s\nGot:\n%s", expected, json.c_str());
}

Test(json_serializer, serialize_null_string) {
    constexpr TestStructStr t{nullptr};

    const std::string json = JsonSerializer::serialize(*TestStructStr::def(), &t);

    const auto expected = "{\n"
            "  \"name\": null\n"
            "}";

    cr_assert_eq(json, expected, "Expected:\n%s\nGot:\n%s", expected, json.c_str());
}

Test(json_serializer, serialize_small_unsigned_types) {
    constexpr TestStructSmallUnsigned t{12, 513};

    const std::string json = JsonSerializer::serialize(*TestStructSmallUnsigned::def(), &t);

    const auto expected = "{\n"
            "  \"flags\": 12,\n"
            "  \"id\": 513\n"
            "}";

    cr_assert_eq(json, expected, "Expected:\n%s\nGot:\n%s", expected, json.c_str());
}

Test(json_serializer, serialize_default_value) {
    struct DefaultStruct {
        int32_t value;
        const char *name;

        DefaultStruct() : value(42), name("default") {
        }

        rayflect(DefaultStruct,
                 DefaultStruct->member<int32_t>("value");
                 DefaultStruct->member<const char *>("name");
        )
    };

    const std::string json = JsonSerializer::serialize_default(
        *DefaultStruct::def(),
        sizeof(DefaultStruct),
        [](void *ptr) {
            *static_cast<DefaultStruct *>(ptr) = DefaultStruct();
        }
    );

    const auto expected = "{\n"
            "  \"value\": 42,\n"
            "  \"name\": \"default\"\n"
            "}";

    cr_assert_eq(json, expected, "Expected:\n%s\nGot:\n%s", expected, json.c_str());
}

Test(json_deserializer, parse_string_value) {
    JsonDeserializer deserializer("\"hello world\"");

    const JsonValue value = deserializer.parse();

    cr_assert_eq(value.type, JsonValueType::String);
    cr_assert_str_eq(value.value.string, "hello world");
}

Test(json_deserializer, parse_number_value) {
    JsonDeserializer deserializer("-42.5");

    const JsonValue value = deserializer.parse();

    cr_assert_eq(value.type, JsonValueType::Number);
    cr_assert_float_eq(value.value.number, -42.5, 1e-6);
}

Test(json_deserializer, parse_boolean_value) {
    JsonDeserializer deserializer("true");

    const JsonValue value = deserializer.parse();

    cr_assert_eq(value.type, JsonValueType::Boolean);
    cr_assert_eq(value.value.boolean, true);
}

Test(json_deserializer, parse_simple_object) {
    JsonDeserializer deserializer("{ \"name\": \"arcade\", \"enabled\": false, \"version\": 3 }");

    const JsonValue value = deserializer.parse();
    const JsonValue *name = value.get("name");
    const JsonValue *enabled = value.get("enabled");
    const JsonValue *version = value.get("version");

    cr_assert_eq(value.type, JsonValueType::Object);
    cr_assert_not_null(name);
    cr_assert_not_null(enabled);
    cr_assert_not_null(version);
    cr_assert_eq(name->type, JsonValueType::String);
    cr_assert_str_eq(name->value.string, "arcade");
    cr_assert_eq(enabled->type, JsonValueType::Boolean);
    cr_assert_eq(enabled->value.boolean, false);
    cr_assert_eq(version->type, JsonValueType::Number);
    cr_assert_float_eq(version->value.number, 3.0, 1e-6);
}

Test(json_deserializer, parse_nested_object) {
    JsonDeserializer deserializer("{\"config\":{\"width\":800,\"title\":\"game\"}}");

    const JsonValue root = deserializer.parse();
    const JsonValue *config = root.get("config");
    const JsonValue *width = config != nullptr ? config->get("width") : nullptr;
    const JsonValue *title = config != nullptr ? config->get("title") : nullptr;

    cr_assert_eq(root.type, JsonValueType::Object);
    cr_assert_not_null(config);
    cr_assert_eq(config->type, JsonValueType::Object);
    cr_assert_not_null(width);
    cr_assert_not_null(title);
    cr_assert_eq(width->type, JsonValueType::Number);
    cr_assert_float_eq(width->value.number, 800.0, 1e-6);
    cr_assert_eq(title->type, JsonValueType::String);
    cr_assert_str_eq(title->value.string, "game");
}

Test(json_deserializer, parse_array_value) {
    JsonDeserializer deserializer("[1, true, \"arcade\", null]");

    const JsonValue value = deserializer.parse();
    const JsonValue *first = value.at(0);
    const JsonValue *second = value.at(1);
    const JsonValue *third = value.at(2);
    const JsonValue *fourth = value.at(3);

    cr_assert_eq(value.type, JsonValueType::Array);
    cr_assert_eq(value.size(), 4);
    cr_assert_not_null(first);
    cr_assert_not_null(second);
    cr_assert_not_null(third);
    cr_assert_not_null(fourth);
    cr_assert_eq(first->type, JsonValueType::Number);
    cr_assert_float_eq(first->value.number, 1.0, 1e-6);
    cr_assert_eq(second->type, JsonValueType::Boolean);
    cr_assert_eq(second->value.boolean, true);
    cr_assert_eq(third->type, JsonValueType::String);
    cr_assert_str_eq(third->value.string, "arcade");
    cr_assert_eq(fourth->type, JsonValueType::Null);
}

Test(json_deserializer, parse_null_value) {
    JsonDeserializer deserializer("null");

    const JsonValue value = deserializer.parse();

    cr_assert_eq(value.type, JsonValueType::Null);
}

Test(json_deserializer, parse_object_with_array_and_null) {
    JsonDeserializer deserializer("{\"items\":[{\"id\":1}, null, false]}");

    const JsonValue root = deserializer.parse();
    const JsonValue *items = root.get("items");
    const JsonValue *first = items != nullptr ? items->at(0) : nullptr;
    const JsonValue *id = first != nullptr ? first->get("id") : nullptr;
    const JsonValue *second = items != nullptr ? items->at(1) : nullptr;
    const JsonValue *third = items != nullptr ? items->at(2) : nullptr;

    cr_assert_eq(root.type, JsonValueType::Object);
    cr_assert_not_null(items);
    cr_assert_eq(items->type, JsonValueType::Array);
    cr_assert_eq(items->size(), 3);
    cr_assert_not_null(first);
    cr_assert_not_null(id);
    cr_assert_eq(id->type, JsonValueType::Number);
    cr_assert_float_eq(id->value.number, 1.0, 1e-6);
    cr_assert_not_null(second);
    cr_assert_eq(second->type, JsonValueType::Null);
    cr_assert_not_null(third);
    cr_assert_eq(third->type, JsonValueType::Boolean);
    cr_assert_eq(third->value.boolean, false);
}
