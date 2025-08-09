#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <SDL2/SDL.h>
#include "modules/autoclicker.h"
#include "timer.h"

// Simple test to debug autoclicker functionality
int main() {
    std::cout << "=== AutoClicker Debug Test ===" << std::endl;
    
    // Create autoclicker instance
    AutoClicker autoclicker;
    
    // Enable it
    autoclicker.enable();
    
    // Test mouse button handling
    std::cout << "Testing mouse button handling..." << std::endl;
    autoclicker.onMouseButton(0, 1); // Left button down
    std::cout << "Left button pressed, isLeftMouseDown should be true" << std::endl;
    
    // Test update loop
    std::cout << "Testing update loop..." << std::endl;
    
    double startTime = Timer::getCurrentTime();
    int clickCount = 0;
    
    for (int i = 0; i < 100; i++) {
        autoclicker.onUpdate();
        
        // Check if we should have clicked
        double currentTime = Timer::getCurrentTime();
        if (currentTime - startTime >= 100) { // After 100ms
            std::cout << "Time elapsed: " << (currentTime - startTime) << "ms" << std::endl;
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    autoclicker.onMouseButton(0, 0); // Left button up
    
    std::cout << "Debug test completed." << std::endl;
    return 0;
}