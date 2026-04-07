#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace datastructures {
    template<typename T, typename Size = uint32_t>
    struct EcsVec {
        T *data = nullptr;
        Size size = 0;
        Size capacity = 0;

        EcsVec() = default;

        EcsVec(Size capacity, T value) : capacity(capacity) {
            this->data = static_cast<T *>(malloc(capacity * sizeof(T)));
            this->capacity = capacity;
            std::fill(this->data, this->data + capacity, value);
        }

        ~EcsVec() {
            ::free(static_cast<void *>(this->data));
        }

        EcsVec(const EcsVec &) = delete;
        EcsVec &operator=(const EcsVec &) = delete;

        EcsVec(EcsVec &&other) noexcept : data(other.data), size(other.size), capacity(other.capacity) {
            other.data = nullptr;
            other.size = 0;
            other.capacity = 0;
        }

        EcsVec &operator=(EcsVec &&other) noexcept {
            if (this != &other) {
                ::free(static_cast<void *>(this->data));
                this->data = other.data;
                this->size = other.size;
                this->capacity = other.capacity;
                other.data = nullptr;
                other.size = 0;
                other.capacity = 0;
            }
            return *this;
        }

        void push_back(const T &value) {
            if (this->size >= this->capacity) {
                this->capacity = this->capacity == 0 ? 1 : this->capacity * 2;
                this->data = static_cast<T *>(realloc(this->data, this->capacity * sizeof(T)));
            }
            std::memcpy(this->data + this->size, &value, sizeof(T));
            this->size += 1;
        }


        void erase(Size index) {
            uint32_t last = this->size - 1;
            if (index != last)
                std::memcpy(this->data + index, this->data + last, sizeof(T));
            this->size -= 1;
        }

        void remove(const T &value) {
            for (uint32_t i = 0; i < this->size; i++) {
                if (this->data[i] == value) {
                    this->erase(i);
                    return;
                }
            }
        }

        bool has(const T &value) {
            for (uint32_t i = 0; i < this->size; i++) {
                if (this->data[i] == value) {
                    return true;
                }
            }
            return false;
        }

        void resize(Size new_size) {
            if (new_size > this->capacity) {
                this->capacity = new_size;
                this->data = static_cast<T *>(realloc(this->data, this->capacity * sizeof(T)));
            }
            this->size = new_size;
        }

        void resize(Size new_size, const T &value) {
            if (new_size > this->capacity) {
                this->capacity = new_size;
                this->data = static_cast<T *>(realloc(this->data, this->capacity * sizeof(T)));
            }
            std::fill(this->data + this->size, this->data + new_size, value);
            this->size = new_size;
        }

        void pop_back() {
            if (this->size > 0) {
                this->size -= 1;
            }
        }

        void free() {
            ::free(this->data);
            this->data = nullptr;
            this->size = 0;
            this->capacity = 0;
        }

        T *begin() {
            return this->data;
        }

        T *end() {
            return this->data + this->size;
        }

        const T *begin() const {
            return this->data;
        }

        const T *end() const {
            return this->data + this->size;
        }

        T &operator[](Size i) {
            return this->data[i];
        }

        const T &operator[](Size i) const {
            return this->data[i];
        }

        [[nodiscard]] bool empty() const {
            return this->size == 0;
        }

        EcsVec clone() const {
            EcsVec<T> clone;
            clone.data = static_cast<T *>(malloc(this->size * sizeof(T)));
            clone.size = this->size;
            clone.capacity = this->size;
            std::memcpy(clone.data, this->data, this->size * sizeof(T));
            return clone;
        }
    };
} // namespace datastructures
