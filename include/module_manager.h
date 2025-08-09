#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>

// Forward declaration
class Module;

// Module category
enum class ModuleCategory {
    COMBAT,
    MOVEMENT,
    RENDER,
    PLAYER,
    WORLD,
    MISC,
    CLIENT
};

// Module information
struct ModuleInfo {
    std::string name;
    std::string description;
    ModuleCategory category;
    int keybind;
    bool enabled;
    bool visible;
    uintptr_t base;
    size_t size;
};

class ModuleManager {
public:
    void tickModules();
private:
    static ModuleManager* instance;
    
public:
    static ModuleManager* getInstance() {
        if (!instance) {
            instance = new ModuleManager();
        }
        return instance;
    }
public:
    ModuleManager();
    ~ModuleManager();
    
    bool initialize();
    void shutdown();
    
    // Module registration
    void registerModules();
    bool registerModule(std::unique_ptr<Module> module);
    bool unregisterModule(const std::string& name);
    
    // Module access
    Module* getModule(const std::string& name);
    std::vector<Module*> getModulesByCategory(ModuleCategory category);
    std::vector<Module*> getAllModules();
    std::vector<std::string> getModuleNames();
    
    // Module control
    bool enableModule(const std::string& name);
    bool disableModule(const std::string& name);
    bool toggleModule(const std::string& name);
    bool isModuleEnabled(const std::string& name);
    
    // Batch operations
    void enableAllModules();
    void disableAllModules();
    void enableCategory(ModuleCategory category);
    void disableCategory(ModuleCategory category);
    
    // Event handling
    void onUpdate();
    void onRender();
    void onKey(int key, bool down);
    void onMouseClick(int button, bool pressed);
    void onPacketSend(void* packet);
    void onPacketReceive(void* packet);
    
    // Configuration
    bool loadConfig(const std::string& configPath);
    bool saveConfig(const std::string& configPath);
    
    // Module discovery
    void loadModulesFromDirectory(const std::string& directory);

private:
    std::unordered_map<std::string, std::unique_ptr<Module>> modules;
    
    // Helper functions
    std::string categoryToString(ModuleCategory category);
    ModuleCategory stringToCategory(const std::string& categoryStr);
};

// Base module class
class Module {
public:
    Module(const std::string& name, const std::string& description, ModuleCategory category);
    virtual ~Module() = default;
    
    // Module info
    const std::string& getName() const { return info.name; }
    const std::string& getDescription() const { return info.description; }
    ModuleCategory getCategory() const { return info.category; }
    bool isEnabled() const { return info.enabled; }
    bool isVisible() const { return info.visible; }
    
    // Module control
    virtual void enable();
    virtual void disable();
    virtual void toggle();
    void setVisible(bool visible) { info.visible = visible; }
    
    // Events (override in derived classes)
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onUpdate() {}
    virtual void onRender() {}
    virtual void onMouseButton(int button, int action) {}
    virtual void onKey(int key, int action) {}
    virtual void onMouseClick(int button, bool pressed) {}
    virtual void onPacketSend(void* packet) {}
    virtual void onPacketReceive(void* packet) {}
    
    // Configuration
    virtual void loadSettings(const std::string& config) {}
    virtual std::string saveSettings() { return ""; }
    
protected:
    ModuleInfo info;
    
    // Helper functions for modules
    void sendChatMessage(const std::string& message);
    void displayNotification(const std::string& message);
}; 