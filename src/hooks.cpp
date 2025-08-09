#include "hooks.h"
#include "hook_manager.h"
#include "client.h"
#include "module_manager.h"
#include "logger.h"
#include "modules/autoclicker.h"
#include "hooks/hooks_1_8_9.h"
#include <SDL2/SDL.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <dlfcn.h>
#include <iostream>

#include <fstream>
#include <sstream>

// Use pointer to avoid static initialization order issues
HookManager* g_hookManager = nullptr;
bool g_show_gui = true;

// Helper function to get hook manager instance
HookManager* getHookManager() {
    if (!g_hookManager) {
        g_hookManager = HookManager::getInstance(0);  // PID not needed when running in same process
    }
    return g_hookManager;
}

// Mouse hook listeners
static std::vector<Module*> g_mouseHookListeners;

// Function pointers to store original functions
typedef int (*SDL_PollEvent_t)(SDL_Event*);
typedef void (*SDL_GL_SwapWindow_t)(SDL_Window*);

SDL_PollEvent_t original_SDL_PollEvent = nullptr;
SDL_GL_SwapWindow_t original_SDL_GL_SwapWindow = nullptr;

int SDL_PollEvent_Hook(SDL_Event* event) {
    // First call the original function to get the actual event
    int result = 0;
    if (original_SDL_PollEvent) {
        result = original_SDL_PollEvent(event);
    } else {
        LOG_ERROR("original_SDL_PollEvent is null, cannot proceed!");
        return 0;
    }
    
    // Then process the event if we got one
    if (result && event) {
        if (event->type == SDL_KEYDOWN) {
            LOG_DEBUG("Key down: " + std::string(SDL_GetKeyName(event->key.keysym.sym)));
            if (event->key.keysym.sym == SDLK_INSERT) {
                g_show_gui = !g_show_gui;
                std::cout << "[SYSTEM] GUI toggled: " << (g_show_gui ? "ON" : "OFF") << std::endl;
                LOG_INFO("GUI toggled: " + std::string(g_show_gui ? "ON" : "OFF"));
            }
            // Add keybind for AutoClicker toggle (R key)
            if (event->key.keysym.sym == SDLK_r) {
                MinecraftClient::getInstance().getModuleManager()->toggleModule("AutoClicker");
                std::cout << "[SYSTEM] AutoClicker toggled" << std::endl;
                LOG_INFO("AutoClicker toggled");
            }
            MinecraftClient::getInstance().getModuleManager()->onKey(event->key.keysym.sym, true);
        } else if (event->type == SDL_KEYUP) {
            LOG_DEBUG("Key up: " + std::string(SDL_GetKeyName(event->key.keysym.sym)));
            MinecraftClient::getInstance().getModuleManager()->onKey(event->key.keysym.sym, false);
        } else if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
            // Notify mouse hook listeners
            int action = (event->type == SDL_MOUSEBUTTONDOWN) ? 1 : 0;
            int button = event->button.button;
            // SDL uses: 1=left, 2=middle, 3=right, 4=side1, 5=side2
            // Convert to 0-based index: 0=left, 1=right, 2=middle, etc.
            int converted_button = button - 1;
            Hooks::notifyMouseButton(converted_button, action);
            LOG_DEBUG("Mouse button: " + std::to_string(converted_button) + " action: " + std::to_string(action));
        }
    }
    
    return result;
}

void SDL_GL_SwapWindow_Hook(SDL_Window* window) {
    // Render our GUI before swapping buffers
    if (g_show_gui) {
        // Render GUI for all enabled modules
        MinecraftClient::getInstance().getModuleManager()->onRender();
    }
    
    // Call the original function to actually swap buffers
    if (original_SDL_GL_SwapWindow) {
        original_SDL_GL_SwapWindow(window);
    }
}

// Function to detect Minecraft version
std::string detect_minecraft_version() {
    // Try to read version from version file
    std::ifstream versionFile("/home/beeny/.minecraft/versions/version.json");
    if (versionFile.is_open()) {
        std::stringstream buffer;
        buffer << versionFile.rdbuf();
        std::string content = buffer.str();
        
        // Look for version string
        size_t pos = content.find("\"id\":");
        if (pos != std::string::npos) {
            size_t start = content.find('"', pos + 5) + 1;
            size_t end = content.find('"', start);
            if (end != std::string::npos) {
                return content.substr(start, end - start);
            }
        }
    }
    
    // Fallback to 1.8.9
    return "1.8.9";
}

void setup_hooks() {
    LOG_INFO("Setting up hooks...");
    
    // Detect Minecraft version
    std::string version = detect_minecraft_version();
    LOG_INFO("Detected Minecraft version: " + version);
    
    // First try to load SDL2 if not already loaded
    void* sdl_handle = dlopen("libSDL2-2.0.so.0", RTLD_LAZY | RTLD_GLOBAL);
    
    // Initialize version-specific hooks
    if (version == "1.8.9") {
        // Get JVM environment
        JavaVM* jvm = nullptr;
        JNIEnv* env = nullptr;
        jint res = JNI_GetCreatedJavaVMs(&jvm, 1, nullptr);
        if (res == JNI_OK && jvm) {
            jvm->AttachCurrentThread((void**)&env, nullptr);
            if (env) {
                Hooks_1_8_9::initialize(env);
            } else {
                LOG_ERROR("Failed to attach to JVM thread");
            }
        } else {
            LOG_ERROR("Failed to get Java VM");
        }
    }
    if (!sdl_handle) {
        LOG_ERROR("Failed to load SDL2 library: " + std::string(dlerror()));
        return;
    }
    LOG_INFO("SDL2 library loaded/found successfully");
    
    // Get SDL function addresses using dlsym
    void* pollEventAddr = dlsym(sdl_handle, "SDL_PollEvent");
    void* swapWindowAddr = dlsym(sdl_handle, "SDL_GL_SwapWindow");
    
    if (pollEventAddr) {
        LOG_INFO("Found SDL_PollEvent at: 0x" + std::to_string((uintptr_t)pollEventAddr));
        if (getHookManager()->createHook("SDL_PollEvent", (uintptr_t)pollEventAddr, (void*)SDL_PollEvent_Hook)) {
            original_SDL_PollEvent = (SDL_PollEvent_t)getHookManager()->getOriginalFunction("SDL_PollEvent");
            LOG_SUCCESS("SDL_PollEvent hooked successfully");
        } else {
            LOG_ERROR("Failed to create SDL_PollEvent hook");
        }
    } else {
        LOG_WARN("Could not find SDL_PollEvent symbol: " + std::string(dlerror()));
    }
    
    if (swapWindowAddr) {
        LOG_INFO("Found SDL_GL_SwapWindow at: 0x" + std::to_string((uintptr_t)swapWindowAddr));
        if (getHookManager()->createHook("SDL_GL_SwapWindow", (uintptr_t)swapWindowAddr, (void*)SDL_GL_SwapWindow_Hook)) {
            original_SDL_GL_SwapWindow = (SDL_GL_SwapWindow_t)getHookManager()->getOriginalFunction("SDL_GL_SwapWindow");
            LOG_SUCCESS("SDL_GL_SwapWindow hooked successfully");
        } else {
            LOG_ERROR("Failed to create SDL_GL_SwapWindow hook");
        }
    } else {
        LOG_WARN("Could not find SDL_GL_SwapWindow symbol: " + std::string(dlerror()));
    }
    
    // Don't close the handle to keep SDL loaded
    // dlclose(sdl_handle);
}

void setup_hooks_pattern_fallback() {
    LOG_INFO("Using pattern-based hook setup as fallback...");
    
    // Store original function pointers before hooking
    // Updated patterns that are more generic
    uintptr_t pollEventAddr = getHookManager()->findPatternAndHook(
        "SDL_PollEvent",
        "libSDL2-2.0.so.0",
        "55 48 89 ?? 48 83 ?? ??",  // Simplified pattern
        "xxx?xx??",
        (void*)SDL_PollEvent_Hook
    );
    
    if (pollEventAddr) {
        original_SDL_PollEvent = (SDL_PollEvent_t)pollEventAddr;
        LOG_SUCCESS("SDL_PollEvent hooked successfully");
    } else {
        LOG_WARN("Failed to hook SDL_PollEvent");
    }
    
    uintptr_t swapWindowAddr = getHookManager()->findPatternAndHook(
        "SDL_GL_SwapWindow",
        "libSDL2-2.0.so.0",
        "55 48 89 ?? 48 83 ?? ??", // Simplified pattern
        "xxx?xx??",
        (void*)SDL_GL_SwapWindow_Hook
    );
    
    if (swapWindowAddr) {
        original_SDL_GL_SwapWindow = (SDL_GL_SwapWindow_t)swapWindowAddr;
        LOG_SUCCESS("SDL_GL_SwapWindow hooked successfully");
    } else {
        LOG_WARN("Failed to hook SDL_GL_SwapWindow");
    }
}

namespace Hooks {
    void registerMouseHook(Module* module) {
        if (std::find(g_mouseHookListeners.begin(), g_mouseHookListeners.end(), module) == g_mouseHookListeners.end()) {
            g_mouseHookListeners.push_back(module);
            LOG_DEBUG("Registered mouse hook for module: " + module->getName());
        }
    }
    
    void unregisterMouseHook(Module* module) {
        auto it = std::find(g_mouseHookListeners.begin(), g_mouseHookListeners.end(), module);
        if (it != g_mouseHookListeners.end()) {
            g_mouseHookListeners.erase(it);
            LOG_DEBUG("Unregistered mouse hook for module: " + module->getName());
        }
    }
    
    void notifyMouseButton(int button, int action) {
        for (Module* module : g_mouseHookListeners) {
            module->onMouseButton(button, action);
        }
    }
    
    void cleanup() {
        if (g_hookManager) {
            // Don't delete here, just set to nullptr since HookManager is a singleton
            g_hookManager = nullptr;
        }
    }
}