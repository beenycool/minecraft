#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unistd.h> // for pid_t

class MemoryManager {
private:
    static MemoryManager* instance;
    
public:
    static MemoryManager* getInstance(pid_t pid = 0) {
        if (!instance) {
            instance = new MemoryManager(pid);
        }
        return instance;
    }
public:
    MemoryManager(pid_t pid);
    uintptr_t getSymbolAddress(const std::string& name);

    bool read(uintptr_t address, void* buffer, size_t size);
    bool write(uintptr_t address, void* buffer, size_t size);

    uintptr_t getModuleBase(const std::string& moduleName);
    uintptr_t patternScan(uintptr_t start, size_t length, const std::string& pattern, const std::string& mask);

    // A convenience wrapper for pattern scanning a whole module
    uintptr_t findPattern(const std::string& moduleName, const std::string& pattern, const std::string& mask);
    
private:
    pid_t pid;

    struct ModuleInfo {
        std::string name;
        uintptr_t base;
        uintptr_t end;
    };

    std::vector<ModuleInfo> getModules();
}; 