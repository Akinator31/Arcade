#pragma once
#include "engine/datastructures/SparseSet.hpp"
#include "engine/reflection/TypeCounter.hpp"


struct Singleton;
using SingletonCounter = reflection::TypeCounter<Singleton>;


class SingletonRegistry {
    datastructures::SparseSet<void *> data;

public:
    template<typename T>
    void singleton_init() {
        const auto value = static_cast<void *>(new T());
        data.set(SingletonCounter::id<T>(), value);
    }

    template<typename T>
    void singleton_init(T *defaultValue) {
        const auto value = static_cast<void *>(defaultValue);
        data.set(SingletonCounter::id<T>(), value);
    }

    template<typename T>
    T *singleton_get() {
        return static_cast<T *>(data.get(SingletonCounter::id<T>()));
    }
};
