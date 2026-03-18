#include "type.hpp"

#include <algorithm>
#include <cstring>

namespace ecs {

void EntityType::sortComponents() {
    std::sort(data, data + count);
}

bool EntityType::has(const uint16_t component) const {
    for (uint8_t i = 0; i < count; ++i) {
        if (data[i] == component) {
            return true;
        }
    }
    return false;
}

void EntityType::add(const uint16_t component) {
    if (has(component)) {
        return;
    }

    const auto newCount = static_cast<uint8_t>(count + 1);
    data = static_cast<ComponentID*>(::realloc(data, static_cast<size_t>(newCount) * sizeof(ComponentID)));
    data[count] = component;
    count = newCount;
    sortComponents();
}

void EntityType::remove(const uint16_t component) {
    uint8_t index = count;
    for (uint8_t i = 0; i < count; ++i) {
        if (data[i] == component) {
            index = i;
            break;
        }
    }

    if (index == count) {
        return;
    }

    const auto tailCount = static_cast<size_t>(count - index - 1);
    if (tailCount > 0) {
        std::memmove(data + index, data + index + 1, tailCount * sizeof(ComponentID));
    }

    --count;
    if (count == 0) {
        ::free(data);
        data = nullptr;
        return;
    }

    data = static_cast<ComponentID*>(::realloc(data, static_cast<size_t>(count) * sizeof(ComponentID)));
}

bool EntityType::operator==(const EntityType& other) const noexcept {
    if (count != other.count) {
        return false;
    }
    if (count == 0) {
        return true;
    }
    return std::memcmp(data, other.data, static_cast<size_t>(count) * sizeof(ComponentID)) == 0;
}

EntityType EntityType::clone() const {
    EntityType copy;
    if (count == 0) {
        return copy;
    }

    copy.data = static_cast<ComponentID*>(::malloc(static_cast<size_t>(count) * sizeof(ComponentID)));
    std::memcpy(copy.data, data, static_cast<size_t>(count) * sizeof(ComponentID));
    copy.count = count;
    return copy;
}

} // namespace ecs