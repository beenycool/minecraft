#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <SDL2/SDL.h>

// Simple test for autoclicker functionality
class TestAutoClicker {
private:
    std::atomic<bool> isLeftMouseDown{false};
    std::atomic<bool> isEnabled{true};
    int targetCPS = 10;
    int consecutiveClicks = 0;
    double nextClickTime = 0;
    bool randomize = true;
    
public:
    void onMouseButton(int button, int action) {
        if (button == 0) { // Left mouse button
            isLeftMouseDown = (action == 1);
            if (!isLeftMouseDown) {
                consecutiveClicks = 0;
            }
            std::cout << "[TEST] Left mouse button " << (action == 1 ? "pressed" : "released") << std::endl;
        }
    }
    
    void onUpdate() {
        if (!isEnabled || !isLeftMouseDown) {
            consecutiveClicks = 0;
            return;
        }
        
        double currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        if (currentTime >= nextClickTime) {
            performClick();
            consecutiveClicks++;
            
            double baseDelay = 1000.0 / targetCPS;
            
            if (randomize) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::normal_distribution<> dist(1.0, 0.1);
                double multiplier = dist(gen);
                if (multiplier < 0.5) multiplier = 0.5;
                if (multiplier > 1.5) multiplier = 1.5;
                baseDelay *= multiplier;
            }
            
            nextClickTime = currentTime + baseDelay;
        }
    }
    
    void performClick() {
        std::cout << "[TEST] AutoClicker: Click! (consecutive: " << consecutiveClicks << ")" << std::endl;
        
        // Simulate mouse click
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.state = SDL_PRESSED;
        SDL_PushEvent(&event);
        
        event.type = SDL_MOUSEBUTTONUP;
        event.button.state = SDL_RELEASED;
        SDL_PushEvent(&event);
    }
};

int main() {
    std::cout << "=== AutoClicker Standalone Test ===" << std::endl;
    std::cout << "This test simulates autoclicker functionality without Minecraft." << std::endl;
    std::cout << "Instructions:" << std::endl;
    std::cout << "1. Hold down left mouse button to activate autoclicking" << std::endl;
    std::cout << "2. Release to stop" << std::endl;
    std::cout << "3. Press Ctrl+C to exit" << std::endl;
    std::cout << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    TestAutoClicker autoclicker;
    
    // Start update thread
    std::atomic<bool> running{true};
    std::thread updateThread([&]() {
        while (running) {
            autoclicker.onUpdate();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    
    // Main event loop
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                int button = 0; // Left button
                int action = (event.type == SDL_MOUSEBUTTONDOWN) ? 1 : 0;
                autoclicker.onMouseButton(button, action);
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
                break;
            }
        }
    }
    
    running = false;
    updateThread.join();
    SDL_Quit();
    
    std::cout << "Test completed." << std::endl;
    return 0;
}