#pragma once

#include "module_manager.h"
#include "hook_manager.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>

// Forward declarations
class ProcessManager;
class MemoryManager;
class HookManager;

class MinecraftClient {
public:
    static MinecraftClient& getInstance();

    MinecraftClient(const MinecraftClient&) = delete;
    MinecraftClient& operator=(const MinecraftClient&) = delete;

    void initialize();
    void shutdown();
    void setup_hooks();
    
    // Getters for managers
    ModuleManager* getModuleManager() { return moduleManager.get(); }
    MemoryManager* getMemoryManager() { return memoryManager.get(); }
    HookManager* getHookManager() { return HookManager::getInstance(); }

    // Input simulation
    void simulateMouseClick(int button, bool down);
    
    // Game state methods
    bool isAimingAtBlock() const;
    bool isHoldingWeapon() const;
    bool isUsingItem() const;
    bool isInventoryOpen() const;
    
    // Rendering methods
    void drawText(const std::string& text, int x, int y);

private:
    MinecraftClient();
    ~MinecraftClient();
    
    bool initialized = false;
    std::unique_ptr<MemoryManager> memoryManager;
    std::unique_ptr<ModuleManager> moduleManager;
    // HookManager is a singleton, so we don't store it here
    
    // Update thread
    std::thread updateThread;
    std::atomic<bool> updateThreadRunning;
    void startUpdateThread();
    void stopUpdateThread();
};