#pragma once
#include <cstdint>
#include <string>
#include <cxxabi.h>
#include <string.h>

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

#define FUNC_LENGTH 28

template<typename T>
char *type_name() {
    std::string func = __PRETTY_FUNCTION__;
    func.erase(0, FUNC_LENGTH);
    func.erase(func.length() - 1, 1);
    return strdup(func.c_str());
}
