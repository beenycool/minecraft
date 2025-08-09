#pragma once

#include "memory_manager.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

class HookManager {
private:
    static HookManager* instance;
    HookManager(pid_t pid);
    
public:
    static HookManager* getInstance(pid_t pid = 0) {
        if (!instance) {
            instance = new HookManager(pid);
        }
        return instance;
    }
    
    static void destroyInstance() {
        if (instance) {
            delete instance;
            instance = nullptr;
        }
    }
    
    void registerHook(const std::string& name, uintptr_t address, std::function<void()> callback);
    ~HookManager();

    bool createHook(const std::string& hookName, uintptr_t targetAddress, void* detourFunction);
    bool removeHook(const std::string& hookName);
    void shutdown();
    void* getOriginalFunction(const std::string& hookName);

    template <typename T>
    T getOriginal(const std::string& hookName) {
        auto it = hooks.find(hookName);
        if (it != hooks.end()) {
            // In a real scenario, this would return a trampoline
            return reinterpret_cast<T>(it->second.targetAddress);
        }
        return nullptr;
    }

    uintptr_t findPatternAndHook(const std::string& hookName, const std::string& moduleName, const std::string& pattern, const std::string& mask, void* detourFunction);

private:
    struct HookInfo {
        uintptr_t targetAddress;
        std::function<void()> callback;
        void* detourFunction;
        void* trampolineFunction;
        std::vector<unsigned char> originalBytes;
    };

    pid_t pid;
    MemoryManager memoryManager;
    std::map<std::string, HookInfo> hooks;
}; 