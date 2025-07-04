#pragma once

#include "module_manager.h"

// Example modules to demonstrate the framework

class SpeedModule : public Module {
public:
    SpeedModule() : Module("Speed", "Allows you to move faster.", ModuleCategory::MOVEMENT) {
        // setKeyBind(VK_F1); // F1 key
    }
    
    void onEnable() override {
        displayNotification("Speed enabled!");
    }
    
    void onDisable() override {
        displayNotification("Speed disabled!");
    }
    
    void onUpdate() override {
        // This would hook into the player movement code
        // For demonstration purposes only
    }
};

class FlightModule : public Module {
public:
    FlightModule() : Module("Flight", "Allows you to fly.", ModuleCategory::MOVEMENT) {
        // setKeyBind(VK_F2); // F2 key
    }
    
    void onEnable() override {
        displayNotification("Flight enabled!");
    }
    
    void onDisable() override {
        displayNotification("Flight disabled!");
    }
    
    void onUpdate() override {
        // This would modify player physics
        // For demonstration purposes only
    }
};

class ESPModule : public Module {
public:
    ESPModule() : Module("ESP", "Allows you to see entities through walls.", ModuleCategory::RENDER) {
        // setKeyBind(VK_F3); // F3 key
    }
    
    void onEnable() override {
        displayNotification("ESP enabled!");
    }
    
    void onDisable() override {
        displayNotification("ESP disabled!");
    }
    
    void onRender() override {
        // This would render entity outlines
        // For demonstration purposes only
    }
};

class AutoClickerModule : public Module {
public:
    AutoClickerModule() : Module("AutoClicker", "Automatically clicks for you.", ModuleCategory::COMBAT) {
        // setKeyBind(VK_F4); // F4 key
    }
    
    void onEnable() override {
        displayNotification("AutoClicker enabled!");
    }
    
    void onDisable() override {
        displayNotification("AutoClicker disabled!");
    }
    
    void onUpdate() override {
        // This would simulate mouse clicks
        // For demonstration purposes only
    }
};

class FullbrightModule : public Module {
public:
    FullbrightModule() : Module("Fullbright", "Allows you to see in the dark.", ModuleCategory::RENDER) {
        // setKeyBind(VK_F5); // F5 key
    }
    
    void onEnable() override {
        displayNotification("Fullbright enabled!");
    }
    
    void onDisable() override {
        displayNotification("Fullbright disabled!");
    }
    
    void onUpdate() override {
        // This would modify the lighting system
        // For demonstration purposes only
    }
}; 