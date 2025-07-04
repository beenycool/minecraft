#include "client.h"
#include "memory_manager.h"
#include "module_manager.h"
#include "hook_manager.h"
#include "hooks.h"
#include "logger.h"
#include <thread>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <mutex>

MinecraftClient& MinecraftClient::getInstance() {
    static MinecraftClient instance;
    return instance;
}

MinecraftClient::MinecraftClient() {
    // Constructor content (if any)
}

MinecraftClient::~MinecraftClient() {
    shutdown();
}

void MinecraftClient::initialize() {
    if (initialized) return;

    try {
        LOG_INFO("MinecraftClient::initialize() called. All initializations are temporarily disabled for debugging.");
        // All initialization logic is commented out for now to find the source of the crash.
    } catch (const std::exception& e) {
        LOG_ERROR("MinecraftClient initialization failed: " + std::string(e.what()));
        initialized = false;
        throw;
    }
}

void MinecraftClient::shutdown() {
    if (!initialized) return;

    LOG_INFO("Minecraft Injectable Client shutting down...");
    
    if (moduleManager) {
        moduleManager.reset();
    }
    
    initialized = false;
    LOG_INFO("MinecraftClient shutdown complete");
}

std::mutex g_hookMutex;

void MinecraftClient::setup_hooks() {
    std::lock_guard<std::mutex> lock(g_hookMutex);
    LOG_DEBUG("Initializing render tick hook");
    LOG_INFO("setup_hooks() called, hook registration temporarily disabled for debugging.");
}

__attribute__((constructor))
void entry() {
    // This function is called when the library is loaded.
    // It's not safe to do much here, especially not logging or complex object creation.
    // We will just trigger the client initialization.
    MinecraftClient::getInstance().initialize();
}

__attribute__((destructor))
void cleanup() {
    // This function is called when the library is unloaded.
    MinecraftClient::getInstance().shutdown();
}