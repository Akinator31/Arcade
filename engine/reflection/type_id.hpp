#pragma once
#include <cstdint>
#include <string>
#include <cxxabi.h>

namespace reflection {
    using TypeId = uint16_t;

    extern "C" TypeId generate_type_id() noexcept;

    template<typename T>
    __attribute__((visibility("default"))) inline TypeId type_id() noexcept {
        static TypeId id = generate_type_id();
        return id;
    }


#include <type_traits>

    template<typename T, typename = void>
    struct is_complete : std::false_type {
    };

    template<typename T>
    struct is_complete<T, std::void_t<decltype(sizeof(T))> > : std::true_type {
    };

    template<typename T>
    constexpr size_t ecs_sizeof() {
        if constexpr (is_complete<T>::value) {
            return sizeof(T);
        } else {
            return 0; // tag
        }
    }
} // namespace reflection

template<typename T>
std::string type_name_string() {
    std::string value = __PRETTY_FUNCTION__;
    const std::string key = "T = ";
    const std::size_t begin = value.find(key);
    if (begin == std::string::npos) {
        return value;
    }
    const std::size_t start = begin + key.size();
    const std::size_t end = value.find_first_of(";]", start);
    if (end == std::string::npos) {
        return value.substr(start);
    }
    return value.substr(start, end - start);
}

template<typename T>
const char *type_name() {
    static const std::string func = type_name_string<T>();
    return func.c_str();
}
