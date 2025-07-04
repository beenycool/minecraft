#include "hooks.h"
#include "hook_manager.h"
#include "client.h"
#include "module_manager.h"
#include "logger.h"
#include <SDL2/SDL.h>
#include <unistd.h>

HookManager g_hookManager(getpid());
bool g_show_gui = true;

int SDL_PollEvent_Hook(SDL_Event* event) {
    if (event) {
        if (event->type == SDL_KEYDOWN) {
            LOG_DEBUG("Key down: " + std::string(SDL_GetKeyName(event->key.keysym.sym)));
            if (event->key.keysym.sym == SDLK_INSERT) {
                g_show_gui = !g_show_gui;
            }
            g_client.getModuleManager()->onKey(event->key.keysym.sym, true);
        } else if (event->type == SDL_KEYUP) {
            LOG_DEBUG("Key up: " + std::string(SDL_GetKeyName(event->key.keysym.sym)));
            g_client.getModuleManager()->onKey(event->key.keysym.sym, false);
        }
    }
    
    // Return the original function result (simplified for now)
    return 1;
}

void SDL_GL_SwapWindow_Hook(SDL_Window* window) {
    if (g_show_gui) {
        // You would render your GUI here
    }
    // Call the original function (simplified for now)
}

void setup_hooks() {
    LOG_INFO("Setting up hooks...");
    // These patterns are examples and might need updating.
    // You can find these patterns using a tool like Slinky
    g_hookManager.findPatternAndHook("SDL_PollEvent", "libSDL2-2.0.so.0", "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC 28 48 89 7D E8", "xxxxxxxxxxxxxxxxxxxxxxxx", (void*)SDL_PollEvent_Hook);
    g_hookManager.findPatternAndHook("SDL_GL_SwapWindow", "libSDL2-2.0.so.0", "55 48 89 e5 41 57 41 56", "xxxxxxxxxxxx", (void*)SDL_GL_SwapWindow_Hook);
} 