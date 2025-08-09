#include "module_manager.h"
void ModuleManager::tickModules() {}
ModuleManager* ModuleManager::instance = nullptr;
#include "module_manager.h"
#include "modules/example_modules.h"
#include "modules/autoclicker.h"
#include "logger.h"
#include "timer.h"
#include <iostream>
#include <algorithm>

ModuleManager::ModuleManager() {
    // Register modules
    registerModule(std::make_unique<AutoClicker>());
}

ModuleManager::~ModuleManager() {
    shutdown();
}

bool ModuleManager::initialize() {
    LOG_INFO("ModuleManager initialized");
    
    // Register example modules
    // Module registration will be done later via registerModules();
    
    LOG_INFO("Registered " + std::to_string(modules.size()) + " modules");
    return true;
}

void ModuleManager::shutdown() {
    disableAllModules();
    modules.clear();
}

bool ModuleManager::registerModule(std::unique_ptr<Module> module) {
    if (!module) {
        return false;
    }
    
    std::string name = module->getName();
    if (modules.find(name) != modules.end()) {
        LOG_WARN("Module '" + name + "' already registered");
        return false;
    }
    
    modules[name] = std::move(module);
    LOG_INFO("Registered module: " + name);
    return true;
}

bool ModuleManager::enableModule(const std::string& name) {
    auto it = modules.find(name);
    if (it == modules.end()) {
        return false;
    }
    
    it->second->enable();
    return true;
}

bool ModuleManager::disableModule(const std::string& name) {
    auto it = modules.find(name);
    if (it == modules.end()) {
        return false;
    }
    
    it->second->disable();
    return true;
}

void ModuleManager::onUpdate() {
    for (const auto& pair : modules) {
        if (pair.second->isEnabled()) {
            pair.second->onUpdate();
        }
    }
}

bool ModuleManager::toggleModule(const std::string& name) {
    auto it = modules.find(name);
    if (it == modules.end()) {
        return false;
    }
    
    it->second->toggle();
    return true;
}

void ModuleManager::disableAllModules() {
    for (const auto& pair : modules) {
        pair.second->disable();
    }
}

void Module::sendChatMessage(const std::string& message) {
    // This functionality requires hooking into Minecraft's chat system.
    // A complete implementation would find the game's chat-sending function
    // and call it with the provided message.
    LOG_INFO("[CHAT] " + message);
}

void Module::displayNotification(const std::string& message) {
    // This functionality requires hooking into Minecraft's rendering loop
    // to draw a custom notification on the screen.
    LOG_INFO("[NOTIFICATION] " + message);
    
    // The current implementation logs to console as a substitute.
    static std::vector<std::pair<std::string, double>> notifications;
    notifications.push_back({message, Timer::getCurrentTime() + 3000}); // Show for 3 seconds
}

// Module base class implementation
Module::Module(const std::string& name, const std::string& description, ModuleCategory category) {
    info.name = name;
    info.description = description;
    info.category = category;
    info.enabled = false;
    info.visible = true;
}

void Module::enable() {
    if (!info.enabled) {
        info.enabled = true;
        onEnable();
        LOG_INFO("Enabled module: " + info.name);
    }
}

void Module::disable() {
    if (info.enabled) {
        info.enabled = false;
        onDisable();
        LOG_INFO("Disabled module: " + info.name);
    }
}

void Module::toggle() {
    if (info.enabled) {
        disable();
    } else {
        enable();
    }
}

void ModuleManager::registerModules() {
    // Register example modules using individual registerModule calls
    registerModule(std::make_unique<SpeedModule>());
    registerModule(std::make_unique<FlightModule>());
    registerModule(std::make_unique<ESPModule>());
    registerModule(std::make_unique<FullbrightModule>());
    LOG_INFO("Registered " + std::to_string(modules.size()) + " modules");
}

void ModuleManager::onKey(int key, bool down) {
    for (auto& [name, module] : modules) {
        if (module->isEnabled()) {
            module->onKey(key, down ? 1 : 0);
        }
    }
}

void ModuleManager::onRender() {
    for (auto& [name, module] : modules) {
        if (module->isEnabled()) {
            module->onRender();
        }
    }
} 