#pragma once

#include <SDL2/SDL.h>

class Module;

namespace Hooks {
    void registerMouseHook(Module* module);
    void unregisterMouseHook(Module* module);
    void notifyMouseButton(int button, int action);
    void cleanup();
}

// Global hook setup function
void setup_hooks();
void setup_hooks_pattern_fallback();