#pragma once
#include "engine/datastructure/SparseSet.hpp"
#include "engine/reflection/TypeCounter.hpp"

struct State;
using StateCounter = reflection::TypeCounter<State>;

class StateRegistry {
    datastructures::SparseSet<uint32_t> states;

public:
    template<typename T>
    void state(T value) {
        this->states.set(StateCounter::id<T>(), static_cast<uint16_t>(value));
    }

    template<typename T>
    T getState() {
        return static_cast<T>(this->states.get(StateCounter::id<T>()));
    }
};

