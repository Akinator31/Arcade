#include "type_id.hpp"

static reflection::TypeId g_type_counter = 0;

extern "C" reflection::TypeId generate_type_id() noexcept {
    return g_type_counter++;
}
