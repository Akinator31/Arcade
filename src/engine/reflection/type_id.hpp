#pragma once
#include <cstdint>

namespace reflection {

using TypeId = uint16_t;
extern "C" TypeId generate_type_id() noexcept;

template <typename T> __attribute__((visibility("default"))) inline TypeId type_id() noexcept {
    static TypeId id = generate_type_id();
    return id;
}

} // namespace reflection
