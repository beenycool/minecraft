#pragma once
#include <cstdint>

// Forward declaration of SDL_Event
union SDL_Event;

// Hook setup function
void setup_hooks();

// SDL Hook Functions (primary)
int SDL_PollEvent_Hook(SDL_Event* event);
void SDL_RenderPresent_Hook(void* renderer);

// Legacy hook functions (for compatibility)
void hk_handle_key_press(int key_code);
void hk_render_overlay(); 