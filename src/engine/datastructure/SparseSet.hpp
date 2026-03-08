#pragma once

#include "EcsVec.hpp"
#include <cstdint>
#include <limits>

namespace datastructures {
template <typename T, typename Size = uint32_t> class SparseSet {
    static constexpr Size kInvalid = std::numeric_limits<Size>::max();

  public:
    [[nodiscard]] bool has(const Size id) const {
        return id < sparse.size && sparse[id] != kInvalid;
    }

    void set(const Size id, const T& value) {
        if (id >= sparse.size)
            sparse.resize(id + 1, kInvalid);

        if (sparse[id] == kInvalid) {
            sparse[id] = dense.size;
            dense.push_back(value);
        } else {
            dense[sparse[id]] = value;
        }
    }

    T& getOrCreate(const Size id) {
        if (id >= sparse.size)
            sparse.resize(id + 1, kInvalid);
        if (sparse[id] == kInvalid) {
            sparse[id] = dense.size;
            dense.push_back(T{});
        }
        return dense[sparse[id]];
    }


    [[nodiscard]] const T& get(const Size id) const {
        return dense[sparse[id]];
    }

    [[nodiscard]] std::size_t size() const {
        return dense.size;
    }

    T& operator[](std::size_t index) {
        return dense[index];
    }

    [[nodiscard]] const EcsVec<T>& getDense() const {
        return this->dense;
    }

    const T& operator[](std::size_t index) const {
        return dense[index];
    }

  private:
    EcsVec<Size, Size> sparse;
    EcsVec<T, Size> dense;
};

} // namespace datastructures
