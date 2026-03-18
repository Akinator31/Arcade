#pragma once

struct Timer {
    float interval;
    float elapsed = 0;

    explicit Timer(const float interval) : interval(interval) {
    }

    bool tick(float dt);
};

template<int interval_ms>
struct TimerTemplate : public Timer {
    static constexpr float interval_sec = interval_ms / 1000.0f;

    TimerTemplate() : Timer(interval_sec) {
    }
};
