#pragma once
#include <cstdint>
#include <vector>

namespace datastructures {

template <typename T> class IndicesAllocator {
    std::vector<T> data;
    std::vector<uint32_t> available;

  public:
    uint32_t alloc() {
        if (!this->available.empty()) {
            const uint32_t result = this->available[this->available.size() - 1];
            this->available.pop_back();
            return result;
        }
        this->data.push_back({});
        return this->data.size() - 1;
    }

    void set(const T& value, uint32_t index) {
        this->data[index] = value;
    }

    uint32_t insert(const T& value) {
      const uint32_t index = this->alloc();
        this->set(value, index);
        return index;
    }

    T& get(uint32_t index) {
        return this->data[index];
    }

    void remove(const uint32_t index) {
        this->available.push_back(index);
    }
};

} // namespace datastructures
