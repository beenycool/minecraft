#pragma once

#include <chrono>
#include <thread>

class Timer {
public:
    static double getCurrentTime() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return static_cast<double>(millis);
    }
    
    static void sleep(double milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(milliseconds)));
    }
};