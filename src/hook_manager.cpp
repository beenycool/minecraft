// Remove duplicate code at the beginning
#include "hook_manager.h"
#include "logger.h"
#include <iostream>
#include <cstring>
#include <sys/mman.h> // For mprotect and mmap

// Initialize static member
HookManager* HookManager::instance = nullptr;

// Simple x86/x64 trampoline size
const int TRAMPOLINE_SIZE = 12;

// Allocate executable memory for trampolines
static void* allocateTrampoline(size_t size) {
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return nullptr;
    }
    return mem;
}

HookManager::HookManager(pid_t pid)
    : pid(pid), memoryManager(pid) {}

HookManager::~HookManager() {
    shutdown();
}

bool HookManager::createHook(const std::string& hookName, uintptr_t targetAddress, void* detourFunction) {
    if (targetAddress == 0) {
        LOG_ERROR("Invalid target address for hook: " + hookName);
        return false;
    }

    auto it = hooks.find(hookName);
    if (it != hooks.end()) {
        LOG_WARN("Hook already exists: " + hookName);
        return false; 
    }

    // Allocate memory for the trampoline function
    size_t trampolineSize = TRAMPOLINE_SIZE + 13; // Original bytes + jump back
    void* trampolineMemory = allocateTrampoline(trampolineSize);
    if (!trampolineMemory) {
        LOG_ERROR("Failed to allocate trampoline memory for hook: " + hookName);
        return false;
    }

    std::vector<unsigned char> originalBytes(TRAMPOLINE_SIZE);
    // Since we're running in the same process after injection, directly read memory
    memcpy(originalBytes.data(), (void*)targetAddress, TRAMPOLINE_SIZE);

    // Create the trampoline
    unsigned char trampolineBytes[TRAMPOLINE_SIZE];
    // mov rax, [detour_address]
    trampolineBytes[0] = 0x48;
    trampolineBytes[1] = 0xb8;
    *(uintptr_t*)&trampolineBytes[2] = (uintptr_t)detourFunction;
    // jmp rax
    trampolineBytes[10] = 0xff;
    trampolineBytes[11] = 0xe0;

    // Change memory protection to RWX for the entire page range
    size_t pageSize = getpagesize();
    uintptr_t startAddr = targetAddress;
    uintptr_t endAddr = targetAddress + TRAMPOLINE_SIZE;
    uintptr_t pageStart = startAddr & ~(pageSize - 1);
    uintptr_t pageEnd = (endAddr + pageSize - 1) & ~(pageSize - 1);
    size_t length = pageEnd - pageStart;

    if (mprotect((void*)pageStart, length, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
        int err = errno;
        LOG_ERROR("mprotect RWX failed for hook: " + hookName + " at 0x" + std::to_string(targetAddress) + " (range: 0x" + std::to_string(pageStart) + "-0x" + std::to_string(pageEnd) + ") - " + strerror(err));
        return false;
    }

    // Write trampoline
    memcpy((void*)targetAddress, trampolineBytes, TRAMPOLINE_SIZE);

    // Restore original protection to RX for the entire page range
    if (mprotect((void*)pageStart, length, PROT_READ | PROT_EXEC) == -1) {
        int err = errno;
        LOG_ERROR("mprotect restore failed for hook: " + hookName + " (range: 0x" + std::to_string(pageStart) + "-0x" + std::to_string(pageEnd) + ") - " + strerror(err));
        // Continue anyway since trampoline was written
    }

    // Create trampoline: copy original bytes + jump back to original function
    unsigned char* trampoline = (unsigned char*)trampolineMemory;
    // Copy original bytes
    memcpy(trampoline, originalBytes.data(), TRAMPOLINE_SIZE);
    // Add jump back to original function + TRAMPOLINE_SIZE
    trampoline[TRAMPOLINE_SIZE] = 0x48; // mov rax
    trampoline[TRAMPOLINE_SIZE + 1] = 0xb8;
    *(uintptr_t*)&trampoline[TRAMPOLINE_SIZE + 2] = targetAddress + TRAMPOLINE_SIZE;
    trampoline[TRAMPOLINE_SIZE + 10] = 0xff; // jmp rax
    trampoline[TRAMPOLINE_SIZE + 11] = 0xe0;

    HookInfo info;
    info.targetAddress = targetAddress;
    info.detourFunction = detourFunction;
    info.originalBytes = originalBytes;
    info.trampolineFunction = trampolineMemory;
    hooks[hookName] = info;
    LOG_SUCCESS("Hook created successfully: " + hookName + " at 0x" + std::to_string(targetAddress));
    return true;
}

bool HookManager::removeHook(const std::string& hookName) {
    auto it = hooks.find(hookName);
    if (it == hooks.end()) {
        LOG_WARN("Attempted to remove non-existent hook: " + hookName);
        return false;
    }

    const auto& hook = it->second;
    
    // Change memory protection to RW for restoring
    size_t pageSize = getpagesize();
    uintptr_t pageStart = hook.targetAddress & ~(pageSize - 1);
    if (mprotect((void*)pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) == 0) {
        memcpy((void*)hook.targetAddress, hook.originalBytes.data(), TRAMPOLINE_SIZE);
        mprotect((void*)pageStart, pageSize, PROT_READ | PROT_EXEC);
    }
    
    // Free trampoline memory
    if (hook.trampolineFunction) {
        munmap(hook.trampolineFunction, TRAMPOLINE_SIZE + 13);
    }
    
    hooks.erase(it);
    LOG_INFO("Hook removed successfully: " + hookName);
    return true;
}

void HookManager::shutdown() {
    if (hooks.empty()) return;

    LOG_INFO("Shutting down HookManager and removing all hooks...");
    for (auto const& [name, hook] : hooks) {
        size_t pageSize = getpagesize();
        uintptr_t pageStart = hook.targetAddress & ~(pageSize - 1);
        if (mprotect((void*)pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) == 0) {
            memcpy((void*)hook.targetAddress, hook.originalBytes.data(), TRAMPOLINE_SIZE);
            mprotect((void*)pageStart, pageSize, PROT_READ | PROT_EXEC);
        }
        // Free trampoline memory
        if (hook.trampolineFunction) {
            munmap(hook.trampolineFunction, TRAMPOLINE_SIZE + 13);
        }
    }
    hooks.clear();
    LOG_INFO("All hooks removed.");
}

void* HookManager::getOriginalFunction(const std::string& hookName) {
    auto it = hooks.find(hookName);
    if (it != hooks.end()) {
        return it->second.trampolineFunction;
    }
    return nullptr;
}

uintptr_t HookManager::findPatternAndHook(const std::string& hookName, const std::string& moduleName, const std::string& pattern, const std::string& mask, void* detourFunction) {
    LOG_INFO("Attempting to find and hook " + hookName);
    uintptr_t address = memoryManager.findPattern(moduleName, pattern, mask);

    if (address != 0) {
        LOG_INFO("Found pattern for '" + hookName + "' at address: 0x" + std::to_string(address));
        if (createHook(hookName, address, detourFunction)) {
            return address;
        } else {
            LOG_ERROR("Failed to create hook: " + hookName);
        }
    } else {
        LOG_WARN("Pattern not found for hook: " + hookName);
    }
    return 0;
}