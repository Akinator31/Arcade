#pragma once
#include "engine/datastructures/SparseSet.hpp"
#include "engine/reflection/TypeCounter.hpp"


struct Singleton;
using SingletonCounter = reflection::TypeCounter<Singleton>;


class SingletonRegistry {
    datastructures::SparseSet<void *> data;
    datastructures::SparseSet<void (*)(void *)> destroyers;

public:
    template<typename T>
    void singleton_init() {
        this->singleton_remove<T>();
        const auto value = static_cast<void *>(new T());
        data.set(SingletonCounter::id<T>(), value);
        destroyers.set(SingletonCounter::id<T>(), [](void *ptr) {
            delete static_cast<T *>(ptr);
        });
    }

    template<typename T>
    void singleton_init(T *defaultValue) {
        this->singleton_remove<T>();
        const auto value = static_cast<void *>(defaultValue);
        data.set(SingletonCounter::id<T>(), value);
        destroyers.set(SingletonCounter::id<T>(), [](void *ptr) {
            delete static_cast<T *>(ptr);
        });
    }

    template<typename T>
    T *singleton_get() {
        if (!this->singleton_has<T>()) {
            return nullptr;
        }
        return static_cast<T *>(data.get(SingletonCounter::id<T>()));
    }

    template<typename T>
    [[nodiscard]] bool singleton_has() const {
        const auto id = SingletonCounter::id<T>();
        return this->data.has(id) && this->data.get(id) != nullptr;
    }

    template<typename T>
    void singleton_remove() {
        const auto id = SingletonCounter::id<T>();
        if (!this->data.has(id)) {
            return;
        }

        if (void *value = this->data.get(id); value != nullptr && this->destroyers.has(id)) {
            if (auto destroy = this->destroyers.get(id); destroy != nullptr) {
                destroy(value);
            }
        }

        this->data.set(id, nullptr);
        this->destroyers.set(id, nullptr);
    }
};
