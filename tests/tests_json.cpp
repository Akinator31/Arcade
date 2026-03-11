#include <criterion/criterion.h>
#include "engine/parsing/JsonSerializer.hpp"
#include "engine/reflection/rayflect.hpp"

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
