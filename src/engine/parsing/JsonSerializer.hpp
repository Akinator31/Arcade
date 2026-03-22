#pragma once

#include "JsonParser.hpp"
#include "engine/reflection/Rayflect.hpp"
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

class JsonSerializer {
public:
    static std::string serialize(const JsonValue &value) {
        std::ostringstream oss;
        serialize_value(oss, value, 0);
        return oss.str();
    }

    static std::string serialize(const StructDef &def, const void *ptr) {
        std::ostringstream oss;
        oss << "{\n";
        const auto &members = def.get_members();
        for (size_t i = 0; i < members.size(); ++i) {
            const auto &m = members[i];
            oss << "  ";
            append_escaped_string(oss, m.name);
            oss << ": ";

            const void *member_ptr = static_cast<const char *>(ptr) + m.offset;
            switch (m.type) {
                case PrimitiveType::u8:
                    oss << static_cast<uint32_t>(*static_cast<const uint8_t *>(member_ptr));
                    break;
                case PrimitiveType::u16:
                    oss << *static_cast<const uint16_t *>(member_ptr);
                    break;
                case PrimitiveType::f32:
                    oss << *static_cast<const float *>(member_ptr);
                    break;
                case PrimitiveType::f64:
                    oss << *static_cast<const double *>(member_ptr);
                    break;
                case PrimitiveType::i32:
                    oss << *static_cast<const int32_t *>(member_ptr);
                    break;
                case PrimitiveType::i64:
                    oss << *static_cast<const int64_t *>(member_ptr);
                    break;
                case PrimitiveType::u32:
                    oss << *static_cast<const uint32_t *>(member_ptr);
                    break;
                case PrimitiveType::u64:
                    oss << *static_cast<const uint64_t *>(member_ptr);
                    break;
                case PrimitiveType::cstr: {
                    const char *str = *static_cast<const char * const*>(member_ptr);
                    if (str) {
                        append_escaped_string(oss, str);
                    } else {
                        oss << "null";
                    }
                    break;
                }
            }

            if (i < members.size() - 1) {
                oss << ",\n";
            } else {
                oss << "\n";
            }
        }
        oss << "}";
        return oss.str();
    }

    template<typename Init>
    static std::string serialize_default(const StructDef &def, const size_t size, Init &&init) {
        std::vector<char> storage(size);
        if (size > 0) {
            std::memset(storage.data(), 0, size);
            init(storage.data());
            return serialize(def, storage.data());
        }
        init(nullptr);
        return serialize(def, nullptr);
    }

private:
    static void append_indent(std::ostringstream &oss, const size_t indent) {
        for (size_t i = 0; i < indent; ++i) {
            oss << ' ';
        }
    }

    static void append_escaped_string(std::ostringstream &oss, const char *str) {
        if (str == nullptr) {
            oss << "null";
            return;
        }

        oss << '"';
        for (const unsigned char c: std::string_view(str)) {
            switch (c) {
                case '\\':
                    oss << "\\\\";
                    break;
                case '"':
                    oss << "\\\"";
                    break;
                case '\b':
                    oss << "\\b";
                    break;
                case '\f':
                    oss << "\\f";
                    break;
                case '\n':
                    oss << "\\n";
                    break;
                case '\r':
                    oss << "\\r";
                    break;
                case '\t':
                    oss << "\\t";
                    break;
                default:
                    if (c < 0x20) {
                        static constexpr char hex[] = "0123456789ABCDEF";
                        oss << "\\u00" << hex[(c >> 4) & 0x0F] << hex[c & 0x0F];
                    } else {
                        oss << static_cast<char>(c);
                    }
                    break;
            }
        }
        oss << '"';
    }

    static void serialize_array(std::ostringstream &oss, const JsonValue &value, const size_t indent) {
        if (value.value.array == nullptr || value.value.array->empty()) {
            oss << "[]";
            return;
        }

        oss << "[\n";
        for (size_t i = 0; i < value.value.array->size(); ++i) {
            append_indent(oss, indent + 2);
            serialize_value(oss, (*value.value.array)[i], indent + 2);
            if (i + 1 < value.value.array->size()) {
                oss << ',';
            }
            oss << '\n';
        }
        append_indent(oss, indent);
        oss << ']';
    }

    static void serialize_object(std::ostringstream &oss, const JsonValue &value, const size_t indent) {
        if (value.value.object == nullptr || value.value.object->empty()) {
            oss << "{}";
            return;
        }

        oss << "{\n";
        for (size_t i = 0; i < value.value.object->size(); ++i) {
            const JsonObjectEntry &entry = (*value.value.object)[i];
            append_indent(oss, indent + 2);
            append_escaped_string(oss, entry.key.get());
            oss << ": ";
            if (entry.value != nullptr) {
                serialize_value(oss, *entry.value, indent + 2);
            } else {
                oss << "null";
            }
            if (i + 1 < value.value.object->size()) {
                oss << ',';
            }
            oss << '\n';
        }
        append_indent(oss, indent);
        oss << '}';
    }

    static void serialize_value(std::ostringstream &oss, const JsonValue &value, const size_t indent) {
        switch (value.type) {
            case JsonValueType::Number:
                oss << value.value.number;
                break;
            case JsonValueType::String:
                append_escaped_string(oss, value.value.string);
                break;
            case JsonValueType::Boolean:
                oss << (value.value.boolean ? "true" : "false");
                break;
            case JsonValueType::Object:
                serialize_object(oss, value, indent);
                break;
            case JsonValueType::Array:
                serialize_array(oss, value, indent);
                break;
            case JsonValueType::Null:
                oss << "null";
                break;
        }
    }
};
