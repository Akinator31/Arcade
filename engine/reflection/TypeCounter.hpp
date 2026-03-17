#pragma once
#include <cstdint>

namespace reflection {

template <typename Family>
class __attribute__((visibility("default"))) TypeCounter {
    static inline uint16_t current_id = 0;
public:
    template <typename T>
    __attribute__((visibility("default"))) static uint16_t id() noexcept {
        static const uint16_t type_id = current_id++;
        return type_id;
    }
};

} // namespace reflection
