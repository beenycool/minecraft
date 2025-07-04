#pragma once

#include "module_manager.h"
#include <memory>
#include <vector>

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

private:
    MinecraftClient();
    ~MinecraftClient();
    
    bool initialized = false;
    std::unique_ptr<ModuleManager> moduleManager;
};

extern MinecraftClient& g_client; 