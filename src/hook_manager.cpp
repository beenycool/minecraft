#include "hook_manager.h"
#include <stdexcept>

HookManager* HookManager::instance = nullptr;

void HookManager::registerHook(const std::string& name, uintptr_t address, std::function<void()> callback) {
    HookInfo info;
    info.targetAddress = address;
    info.callback = callback;
    hooks[name] = info;
    // Dummy implementation for testing
}
#include "hook_manager.h"
#include "logger.h"
#include <iostream>
#include <cstring>
#include <sys/mman.h> // For mprotect

// Simple x86/x64 trampoline size
const int TRAMPOLINE_SIZE = 12;

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

    std::vector<unsigned char> originalBytes(TRAMPOLINE_SIZE);
    if (!memoryManager.read(targetAddress, originalBytes.data(), TRAMPOLINE_SIZE)) {
        LOG_ERROR("Failed to read original bytes for hook: " + hookName);
        return false;
    }

    // Create the trampoline
    unsigned char trampolineBytes[TRAMPOLINE_SIZE];
    // mov rax, [detour_address]
    trampolineBytes[0] = 0x48;
    trampolineBytes[1] = 0xb8;
    *(uintptr_t*)&trampolineBytes[2] = (uintptr_t)detourFunction;
    // jmp rax
    trampolineBytes[10] = 0xff;
    trampolineBytes[11] = 0xe0;

    if (!memoryManager.write(targetAddress, trampolineBytes, TRAMPOLINE_SIZE)) {
        LOG_ERROR("Failed to write trampoline for hook: " + hookName);
        return false;
    }

    HookInfo info;
    info.targetAddress = targetAddress;
    // If HookInfo has more members, assign them here as needed.
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
    if (!memoryManager.write(hook.targetAddress, (void*)hook.originalBytes.data(), TRAMPOLINE_SIZE)) {
        LOG_ERROR("Failed to restore original bytes for hook: " + hookName);
        return false;
    }

    hooks.erase(it);
    LOG_INFO("Hook removed successfully: " + hookName);
    return true;
}

void HookManager::shutdown() {
    if (hooks.empty()) return;

    LOG_INFO("Shutting down HookManager and removing all hooks...");
    for (auto const& [name, hook] : hooks) {
        if (!memoryManager.write(hook.targetAddress, (void*)hook.originalBytes.data(), TRAMPOLINE_SIZE)) {
            LOG_ERROR("Failed to restore original bytes for hook: " + name);
        }
    }
    hooks.clear();
    LOG_INFO("All hooks removed.");
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