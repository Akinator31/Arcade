#include "Timer.hpp"

bool Timer::tick(const float dt) {
    elapsed += dt;

    if (elapsed >= interval) {
        elapsed -= interval;
        return true;
    }
    return false;
}
