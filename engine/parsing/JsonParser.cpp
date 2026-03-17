#include "JsonParser.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>

void JsonFreeDeleter::operator()(const char *ptr) const {
    std::free(const_cast<char *>(ptr));
}

JsonValue::JsonValue(JsonValue &&other) noexcept : type(other.type), value(other.value) {
    other.type = JsonValueType::Null;
    other.value.object = nullptr;
}

JsonValue &JsonValue::operator=(JsonValue &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    this->reset();
    this->type = other.type;
    this->value = other.value;
    other.type = JsonValueType::Null;
    other.value.object = nullptr;
    return *this;
}

JsonValue::~JsonValue() {
    this->reset();
}

JsonValue JsonValue::makeNull() {
    return {};
}

JsonValue JsonValue::makeNumber(const double number) {
    JsonValue result;
    result.type = JsonValueType::Number;
    result.value.number = number;
    return result;
}

JsonValue JsonValue::makeBoolean(const bool boolean) {
    JsonValue result;
    result.type = JsonValueType::Boolean;
    result.value.boolean = boolean;
    return result;
}

JsonValue JsonValue::makeString(const char *string) {
    JsonValue result;
    result.type = JsonValueType::String;
    result.value.string = string;
    return result;
}

JsonValue JsonValue::makeObject() {
    JsonValue result;
    result.type = JsonValueType::Object;
    result.value.object = new std::vector<JsonObjectEntry>();
    return result;
}

JsonValue JsonValue::makeArray() {
    JsonValue result;
    result.type = JsonValueType::Array;
    result.value.array = new std::vector<JsonValue>();
    return result;
}

const JsonValue *JsonValue::get(const char *key) const {
    if (this->type != JsonValueType::Object || this->value.object == nullptr) {
        return nullptr;
    }
    for (const JsonObjectEntry &entry: *this->value.object) {
        if (entry.key != nullptr && std::strcmp(entry.key.get(), key) == 0) {
            return entry.value.get();
        }
    }
    return nullptr;
}

const JsonValue *JsonValue::at(const size_t index) const {
    if (this->type != JsonValueType::Array || this->value.array == nullptr || index >= this->value.array->size()) {
        return nullptr;
    }
    return &(*this->value.array)[index];
}

size_t JsonValue::size() const {
    if (this->type == JsonValueType::Array && this->value.array != nullptr) {
        return this->value.array->size();
    }
    if (this->type == JsonValueType::Object && this->value.object != nullptr) {
        return this->value.object->size();
    }
    return 0;
}

void JsonValue::reset() {
    if (this->type == JsonValueType::String) {
        std::free(const_cast<char *>(this->value.string));
    } else if (this->type == JsonValueType::Object && this->value.object != nullptr) {
        delete this->value.object;
    } else if (this->type == JsonValueType::Array && this->value.array != nullptr) {
        delete this->value.array;
    }

    this->type = JsonValueType::Null;
    this->value.object = nullptr;
}

JsonDeserializer::JsonDeserializer(const char *content) : scanner(content) {
}

std::optional<const char *> JsonDeserializer::parseValueString() {
    if (!scanner.expect('"')) {
        return std::nullopt;
    }
    scanner.advance();
    const std::string result = scanner.take_while([](char c) { return c != '"'; });
    if (scanner.isDone() || !scanner.expect('"')) {
        return std::nullopt;
    }
    scanner.advance();
    return strdup(result.c_str());
}

std::optional<double> JsonDeserializer::parseValueNumber() {
    return scanner.take_value<double>();
}

std::optional<bool> JsonDeserializer::parseValueBool() {
    if (scanner.expect("true")) {
        return true;
    }
    if (scanner.expect("false")) {
        return false;
    }
    return std::nullopt;
}

std::optional<std::nullptr_t> JsonDeserializer::parseValueNull() {
    if (scanner.expect("null")) {
        return nullptr;
    }
    return std::nullopt;
}

std::optional<JsonValue> JsonDeserializer::parseValueObject() {
    if (!scanner.expect('{')) {
        return std::nullopt;
    }
    scanner.advance();
    scanner.skip_whitespace();

    JsonValue object = JsonValue::makeObject();

    if (scanner.expect('}')) {
        scanner.advance();
        return object;
    }

    while (!scanner.isDone()) {
        auto key = this->parseValueString();
        if (!key.has_value()) {
            object = JsonValue::makeNull();
            return std::nullopt;
        }
        JsonCString owned_key(key.value());

        scanner.skip_whitespace();
        if (!scanner.expect(':')) {
            object = JsonValue::makeNull();
            return std::nullopt;
        }
        scanner.advance();
        scanner.skip_whitespace();

        auto child = this->parseAnyValue();
        if (!child.has_value()) {
            object = JsonValue::makeNull();
            return std::nullopt;
        }

        auto stored_value = std::make_unique<JsonValue>(std::move(child.value()));
        object.value.object->push_back({
            .key = std::move(owned_key),
            .value = std::move(stored_value)
        });

        scanner.skip_whitespace();
        if (scanner.expect('}')) {
            scanner.advance();
            return object;
        }
        if (!scanner.expect(',')) {
            object = JsonValue::makeNull();
            return std::nullopt;
        }
        scanner.advance();
        scanner.skip_whitespace();
    }

    object = JsonValue::makeNull();
    return std::nullopt;
}

std::optional<JsonValue> JsonDeserializer::parseValueArray() {
    if (!scanner.expect('[')) {
        return std::nullopt;
    }
    scanner.advance();
    scanner.skip_whitespace();

    JsonValue array = JsonValue::makeArray();

    if (scanner.expect(']')) {
        scanner.advance();
        return array;
    }

    while (!scanner.isDone()) {
        auto child = this->parseAnyValue();
        if (!child.has_value()) {
            array = JsonValue::makeNull();
            return std::nullopt;
        }
        array.value.array->push_back(std::move(child.value()));

        scanner.skip_whitespace();
        if (scanner.expect(']')) {
            scanner.advance();
            return array;
        }
        if (!scanner.expect(',')) {
            array = JsonValue::makeNull();
            return std::nullopt;
        }
        scanner.advance();
        scanner.skip_whitespace();
    }

    array = JsonValue::makeNull();
    return std::nullopt;
}

std::optional<JsonValue> JsonDeserializer::parseAnyValue() {
    scanner.skip_whitespace();

    if (scanner.isDone()) {
        return std::nullopt;
    }

    if (scanner.expect('"')) {
        if (const auto result = this->parseValueString(); result.has_value()) {
            return JsonValue::makeString(result.value());
        }
        return std::nullopt;
    }

    if (scanner.expect('{')) {
        return this->parseValueObject();
    }

    if (scanner.expect('[')) {
        return this->parseValueArray();
    }

    if (scanner.expect('t') || scanner.expect('f')) {
        if (const auto result = this->parseValueBool(); result.has_value()) {
            return JsonValue::makeBoolean(result.value());
        }
        return std::nullopt;
    }

    if (scanner.expect('n')) {
        if (this->parseValueNull().has_value()) {
            return JsonValue::makeNull();
        }
        return std::nullopt;
    }

    if (scanner.expect([](const char c) { return std::isdigit(c) || c == '-'; })) {
        if (const auto result = this->parseValueNumber(); result.has_value()) {
            return JsonValue::makeNumber(result.value());
        }
        return std::nullopt;
    }

    return std::nullopt;
}

JsonValue JsonDeserializer::parse() {
    if (auto result = this->parseAnyValue(); result.has_value()) {
        return std::move(result.value());
    }
    return JsonValue::makeNull();
}
