#include "modules/autoclicker.h"
#include "client.h"
#include "hooks.h"
#include "timer.h"
#include "logger.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <iostream>
#include <atomic>
#include <SDL2/SDL.h>

using json = nlohmann::json;

// Use an atomic flag to prevent race conditions between the game thread (input) and our update thread.
// This is key to preventing the autoclicker from processing its own simulated events.
static std::atomic<bool> isSimulatingClick = {false};

AutoClicker::AutoClicker() : Module("AutoClicker", "Automatically clicks for you", ModuleCategory::COMBAT) {
    // Load default configuration
    loadConfig();
}

void AutoClicker::loadConfig() {
    try {
        std::ifstream file("config/modules/autoclicker.json");
        if (!file.is_open()) {
            LOG_WARN("AutoClicker config not found, using defaults");
            return;
        }

        json config;
        file >> config;

        if (config.contains("clickPattern")) {
            std::string pattern = config["clickPattern"];
            if (pattern == "butterfly") {
                clickPattern = BUTTERFLY;
            } else {
                clickPattern = JITTER;
            }
        }

        if (config.contains("targetCPS")) targetCPS = config["targetCPS"];
        if (config.contains("randomize")) randomize = config["randomize"];
        if (config.contains("randomizeMean")) randomizeMean = config["randomizeMean"];
        if (config.contains("randomizeStdDev")) randomizeStdDev = config["randomizeStdDev"];
        if (config.contains("simulateExhaust")) simulateExhaust = config["simulateExhaust"];
        if (config.contains("allowBreakingBlocks")) allowBreakingBlocks = config["allowBreakingBlocks"];
        if (config.contains("allowInventory")) allowInventory = config["allowInventory"];
        if (config.contains("randomizeInventorySpeed")) randomizeInventorySpeed = config["randomizeInventorySpeed"];
        if (config.contains("holdingWeapon")) holdingWeapon = config["holdingWeapon"];
        if (config.contains("notUsingItem")) notUsingItem = config["notUsingItem"];

        LOG_INFO("AutoClicker config loaded");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load AutoClicker config: " + std::string(e.what()));
    }
}

void AutoClicker::onEnable() {
    Hooks::registerMouseHook(this);
    loadConfig();
    std::cout << "[AUTOCLICKER] AutoClicker enabled" << std::endl;
    LOG_INFO("AutoClicker enabled");
}

void AutoClicker::onDisable() {
    Hooks::unregisterMouseHook(this);
    std::cout << "[AUTOCLICKER] AutoClicker disabled" << std::endl;
    LOG_INFO("AutoClicker disabled");
}

void AutoClicker::onUpdate() {
    updateConditionals();
    
    // Only proceed if left mouse button is down and conditions are met
    if (!isLeftMouseDown || !checkConditionals()) {
        consecutiveClicks = 0;
        return;
    }
    
    LOG_DEBUG("AutoClicker: Active - left button down, conditionals met");
    
    double currentTime = Timer::getCurrentTime();
    if (currentTime >= nextClickTime) {
        if (clickPattern == BUTTERFLY && consecutiveClicks % 2 == 0) {
            performDoubleClick();
        } else {
            performClick();
        }
        
        consecutiveClicks++;
        nextClickTime = currentTime + calculateNextDelay();
    }
}

void AutoClicker::onMouseButton(int button, int action) {
    // This is the core fix for the recursion bug.
    // When we simulate a click, we set isSimulatingClick to true.
    // The hook will pick up our simulated click and call this function.
    // We check the flag to see if it's our own click.
    if (isSimulatingClick.load(std::memory_order_acquire)) {
        // If it's a simulated mouse up, we reset the flag.
        if (action == 0) { // Mouse up
             isSimulatingClick.store(false, std::memory_order_release);
        }
        return; // Ignore the event for state-changing purposes.
    }

    if (button == 0) { // Left mouse button
        isLeftMouseDown = (action == 1);
        if (!isLeftMouseDown) {
            consecutiveClicks = 0;
        }
        std::cout << "[AUTOCLICKER] Left mouse button " << (action == 1 ? "pressed" : "released") << std::endl;
        LOG_DEBUG("AutoClicker: Left mouse button " + std::string(action == 1 ? "pressed" : "released"));
    }
}

void AutoClicker::onKey(int key, int action) {
    // FIX: Use correct SDL keycodes for Shift instead of magic numbers.
    if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
        shiftKeyDown = (action == 1 || action == 2); // Pressed or repeated
    }
}

bool AutoClicker::checkConditionals() {
    // Check if we're aiming at a block and breaking is not allowed
    if (!allowBreakingBlocks && MinecraftClient::getInstance().isAimingAtBlock()) {
        return false;
    }
    
    // Check inventory condition. This allows clicking in inventory only if shift is also held.
    if (inInventory && !(allowInventory && shiftKeyDown)) {
        return false;
    }
    
    // Check if we need to be holding a weapon
    if (holdingWeapon && !MinecraftClient::getInstance().isHoldingWeapon()) {
        return false;
    }
    
    // Check if we shouldn't be using an item
    if (notUsingItem && MinecraftClient::getInstance().isUsingItem()) {
        return false;
    }
    
    return true;
}

void AutoClicker::performClick() {
    isSimulatingClick.store(true, std::memory_order_release);
    LOG_DEBUG("[AUTOCLICKER] Performing click");
    MinecraftClient::getInstance().simulateMouseClick(0, true);
    MinecraftClient::getInstance().simulateMouseClick(0, false);
    // The `isSimulatingClick` flag is reset to false inside onMouseButton now.
}

void AutoClicker::performDoubleClick() {
    isSimulatingClick.store(true, std::memory_order_release);
    // Simulate double click
    MinecraftClient::getInstance().simulateMouseClick(0, true);
    MinecraftClient::getInstance().simulateMouseClick(0, false);
    // A tiny delay can make double clicks more reliable in some games.
    Timer::sleep(10);
    MinecraftClient::getInstance().simulateMouseClick(0, true);
    MinecraftClient::getInstance().simulateMouseClick(0, false);
    // The flag is reset by the final mouse up event.
}

double AutoClicker::calculateNextDelay() {
    double baseDelay = 1000.0 / targetCPS;
    
    if (randomize) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> dist(randomizeMean, randomizeStdDev);
        double multiplier = dist(gen);
        // Clamp multiplier to avoid extreme values
        if (multiplier < 0.5) multiplier = 0.5;
        if (multiplier > 1.5) multiplier = 1.5;
        baseDelay *= multiplier;
    }
    
    // Simulate player exhaust after many clicks
    if (simulateExhaust && consecutiveClicks > 20) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(1.5, 3.0);
        baseDelay *= dist(gen);
    }
    
    // Slow down in inventory
    if (inInventory && randomizeInventorySpeed) {
        baseDelay = std::max(baseDelay, 100.0); // Minimum 100ms delay in inventory
    }
    
    return baseDelay;
}

void AutoClicker::updateConditionals() {
    inInventory = MinecraftClient::getInstance().isInventoryOpen();
}

void AutoClicker::onRender() {
    if (!isEnabled()) return;

    // Simple text-based GUI for demonstration
    // In a real implementation, this would use ImGui
    MinecraftClient::getInstance().drawText("AutoClicker: " + std::string(isEnabled() ? "ON" : "OFF"), 10, 50);
    MinecraftClient::getInstance().drawText("Pattern: " + std::string(clickPattern == JITTER ? "Jitter" : "Butterfly"), 10, 70);
    MinecraftClient::getInstance().drawText("CPS: " + std::to_string(targetCPS), 10, 90);
    MinecraftClient::getInstance().drawText("Random: " + std::string(randomize ? "Gaussian" : "Off"), 10, 110);
    if (randomize) {
        MinecraftClient::getInstance().drawText("Mean: " + std::to_string(randomizeMean), 10, 130);
        MinecraftClient::getInstance().drawText("StdDev: " + std::to_string(randomizeStdDev), 10, 150);
    }
}