#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "Scanner.hpp"

enum class JsonValueType {
    Number,
    String,
    Boolean,
    Object,
    Array,
    Null
};

struct JsonValue;

struct JsonFreeDeleter {
    void operator()(const char *ptr) const;
};

using JsonCString = std::unique_ptr<const char, JsonFreeDeleter>;

struct JsonObjectEntry {
    JsonCString key{};
    std::unique_ptr<JsonValue> value{};
};

union JsonValueStorage {
    double number;
    bool boolean;
    const char *string;
    std::vector<JsonObjectEntry> *object;
    std::vector<JsonValue> *array;

    JsonValueStorage() : object(nullptr) {
    }
};

struct JsonValue {
    JsonValueType type{JsonValueType::Null};
    JsonValueStorage value;

    JsonValue() = default;
    JsonValue(const JsonValue &) = delete;
    JsonValue &operator=(const JsonValue &) = delete;
    JsonValue(JsonValue &&other) noexcept;
    JsonValue &operator=(JsonValue &&other) noexcept;
    ~JsonValue();

    static JsonValue makeNull();
    static JsonValue makeNumber(double number);
    static JsonValue makeBoolean(bool boolean);
    static JsonValue makeString(const char *string);
    static JsonValue makeObject();
    static JsonValue makeArray();

    [[nodiscard]] const JsonValue *get(const char *key) const;
    [[nodiscard]] const JsonValue *at(size_t index) const;
    [[nodiscard]] size_t size() const;

private:
    void reset();
};

struct JsonDeserializer {
    Scanner scanner;

public:
    explicit JsonDeserializer(const char *content);

    JsonValue parse();
    std::optional<const char *> parseValueString();
    std::optional<double> parseValueNumber();
    std::optional<bool> parseValueBool();
    std::optional<std::nullptr_t> parseValueNull();
    std::optional<JsonValue> parseValueObject();
    std::optional<JsonValue> parseValueArray();

private:
    std::optional<JsonValue> parseAnyValue();
};
