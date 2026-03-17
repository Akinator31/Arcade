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
