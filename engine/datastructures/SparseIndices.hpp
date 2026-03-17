#pragma once

#include "EcsVec.hpp"
#include <cstdint>

namespace datastructures {

class SparseIndices {
    EcsVec<uint16_t, uint16_t> ids;

  public:
    explicit SparseIndices(const uint16_t size = 0) : ids(size, UINT16_MAX) {
    }

    [[nodiscard]] bool has(const uint16_t index) const noexcept {
        return index < ids.size && ids[index] != UINT16_MAX;
    }

    void set(const uint16_t index, const uint16_t id) noexcept {
        if (index >= ids.size) {
            ids.resize(index + 1, UINT16_MAX);
        }
        ids[index] = id;
    }

    [[nodiscard]] uint16_t get(const uint16_t index) const {
        if (index >= ids.size) {
            return UINT16_MAX;
        }
        return ids[index];
    }
};

} // namespace datastructures
