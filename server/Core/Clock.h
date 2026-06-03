#pragma once

#include <Windows.h>
#include <cstdint>

class Clock {
public:
    static int64_t Now() {
        static LARGE_INTEGER freq = []() {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return f;
        }();
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return (counter.QuadPart * 1000000LL) / freq.QuadPart;
    }

    static float SecondsSince(int64_t startUs) {
        return static_cast<float>(Now() - startUs) / 1000000.0f;
    }
};
