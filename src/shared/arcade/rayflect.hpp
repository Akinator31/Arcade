#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

enum class PrimitiveType : int8_t {
    u8,
    u16,
    u32,
    u64,
    i32,
    i64,
    f32,
    f64,
    cstr,
};

struct Primitive {
    PrimitiveType type;

    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        const char *cstr;
    } value;

#define methode(name, type) static Primitive name(const type value) { return {PrimitiveType::name, {.name = value}}; }

    methode(u8, uint8_t)
    methode(u16, uint16_t)
    methode(f32, float)
    methode(f64, double)
    methode(i32, int32_t)
    methode(i64, int64_t)
    methode(u32, uint32_t)
    methode(u64, uint64_t)
    static Primitive cstr(const char *value) { return {PrimitiveType::cstr, {.cstr = value}}; }
};

struct StructMember {
    const char *name;
    PrimitiveType type;
    size_t offset;
    size_t size;
    size_t align;
};

class StructDef {
    std::vector<StructMember> members;
    size_t struct_size = 0;
    size_t struct_align = 1;

    static size_t align_up(const size_t v, const size_t a) {
        return (v + a - 1) & ~(a - 1);
    }

    const StructMember *find(const char *name) const {
        for (auto &m: members) {
            if (std::strcmp(m.name, name) == 0) {
                return &m;
            }
        }
        return nullptr;
    }

public:
    template<typename T>
    StructDef *member(const char *name) {
        StructMember m{};
        m.name = name;
        m.size = sizeof(T);
        m.align = alignof(T);

        if constexpr (std::is_same_v<T, uint8_t>) m.type = PrimitiveType::u8;
        else if constexpr (std::is_same_v<T, uint16_t>) m.type = PrimitiveType::u16;
        else if constexpr (std::is_same_v<T, float>) m.type = PrimitiveType::f32;
        else if constexpr (std::is_same_v<T, double>) m.type = PrimitiveType::f64;
        else if constexpr (std::is_same_v<T, int>) m.type = PrimitiveType::i32;
        else if constexpr (std::is_same_v<T, long>) m.type = PrimitiveType::i64;
        else if constexpr (std::is_same_v<T, uint32_t>) m.type = PrimitiveType::u32;
        else if constexpr (std::is_same_v<T, uint64_t>) m.type = PrimitiveType::u64;
        else if constexpr (std::is_same_v<T, const char *>) m.type = PrimitiveType::cstr;

        struct_size = align_up(struct_size, m.align);
        m.offset = struct_size;

        struct_size += m.size;
        struct_align = std::max(struct_align, m.align);

        members.push_back(m);
        return this;
    }

    template<typename T>
    T get_member_value(void *ptr, const char *name) const {
        const auto m = find(name);
        if (!m) throw;

        return *reinterpret_cast<T *>(static_cast<char *>(ptr) + m->offset);
    }

    template<typename T>
    void set_member_value(void *ptr, const char *name, T value) const {
        const auto m = find(name);
        if (!m) throw;

        *reinterpret_cast<T *>(static_cast<char *>(ptr) + m->offset) = value;
    }

    void from_string(void *ptr, const char *name, const std::string &value) const {
        const auto m = find(name);
        if (!m) {
            return;
        }

        void *member_ptr = static_cast<char *>(ptr) + m->offset;

        try {
            switch (m->type) {
                case PrimitiveType::u8:
                    *static_cast<uint8_t *>(member_ptr) = static_cast<uint8_t>(std::stoul(value));
                    break;
                case PrimitiveType::u16:
                    *static_cast<uint16_t *>(member_ptr) = static_cast<uint16_t>(std::stoul(value));
                    break;
                case PrimitiveType::f32:
                    *static_cast<float *>(member_ptr) = std::stof(value);
                    break;
                case PrimitiveType::f64:
                    *static_cast<double *>(member_ptr) = std::stod(value);
                    break;
                case PrimitiveType::i32:
                    *static_cast<int32_t *>(member_ptr) = std::stoi(value);
                    break;
                case PrimitiveType::i64:
                    *static_cast<int64_t *>(member_ptr) = std::stoll(value);
                    break;
                case PrimitiveType::u32:
                    *static_cast<uint32_t *>(member_ptr) = std::stoul(value);
                    break;
                case PrimitiveType::u64:
                    *static_cast<uint64_t *>(member_ptr) = std::stoull(value);
                    break;
                case PrimitiveType::cstr: {
                    const auto str = new char[value.length() + 1];
                    std::strcpy(str, value.c_str());
                    *static_cast<const char **>(member_ptr) = str;
                    break;
                }
            }
        } catch (...) {
        }
    }

    [[nodiscard]] const std::vector<StructMember> &get_members() const {
        return members;
    }

    [[nodiscard]] size_t size() const {
        return align_up(struct_size, struct_align);
    }
};

#define rayflect(name, ...) \
static StructDef *def() { \
    static StructDef name##_storage{}; \
    StructDef *name = &name##_storage; \
    static bool initialized = false; \
    if (!initialized) { \
        __VA_ARGS__ \
        initialized = true; \
    } \
    return name; \
}

template<typename T>
concept HasDef = requires() {
    { T::def() } -> std::same_as<StructDef *>;
};
