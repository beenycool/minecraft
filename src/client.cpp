#include "client.h"
#include "memory_manager.h"
#include "module_manager.h"
#include "hook_manager.h"
#include "hooks.h"
#include "logger.h"
#include "timer.h"
#include <thread>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <mutex>
#include <atomic>
#include <iostream>
#include <SDL2/SDL.h>
#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#endif

MinecraftClient& MinecraftClient::getInstance() {
    static MinecraftClient instance;
    return instance;
}

MinecraftClient::MinecraftClient() : updateThreadRunning(false) {
    // Constructor content (if any)
}

MinecraftClient::~MinecraftClient() {
    shutdown();
}

void MinecraftClient::initialize() {
    if (initialized) return;

    try {
        LOG_INFO("Initializing MinecraftClient...");
        
        // An injected library should NOT initialize or quit SDL.
        // The host application (Minecraft) owns the SDL lifecycle.
        
        // Initialize memory manager
        memoryManager = std::make_unique<MemoryManager>(getpid());
        LOG_INFO("Memory manager initialized");
        
        // Initialize module manager
        moduleManager = std::make_unique<ModuleManager>();
        if (!moduleManager->initialize()) {
            throw std::runtime_error("Failed to initialize module manager");
        }
        LOG_INFO("Module manager initialized");
        
        // Initialize hook manager (singleton - don't create new instance)
        HookManager::getInstance(getpid());  // Initialize singleton
        LOG_INFO("Hook manager initialized");
        
        // Setup hooks
        setup_hooks();
        
        // Start update thread
        startUpdateThread();
        
        // Enable AutoClicker module by default
        moduleManager->enableModule("AutoClicker");
        LOG_INFO("AutoClicker module enabled by default");
        
        initialized = true;
        LOG_SUCCESS("MinecraftClient initialized successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("MinecraftClient initialization failed: " + std::string(e.what()));
        initialized = false;
        throw;
    }
}

void MinecraftClient::shutdown() {
    if (!initialized) return;

    LOG_INFO("Minecraft Injectable Client shutting down...");
    
    // Stop update thread
    stopUpdateThread();
    
    // Cleanup hooks before shutting down modules
    Hooks::cleanup();
    HookManager::destroyInstance();  // Destroy the singleton instance
    
    if (moduleManager) {
        moduleManager.reset();
    }
    
    // Do NOT call SDL_Quit(). The host application owns SDL.
    // Quitting it here would shut down video/input for the entire game.
    
    initialized = false;
    LOG_INFO("MinecraftClient shutdown complete");
}

// ... rest of the file ...

void MinecraftClient::drawText(const std::string& text, int x, int y) {
    // Minimal safe implementation: log to our file; avoid interfering with host renderer.
    // In future, this can be replaced with in-game overlay rendering (e.g., ImGui or OpenGL hooks).
    LOG_INFO("DrawText request: '" + text + "' at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

void MinecraftClient::simulateMouseClick(int button, bool down) {
#ifdef __linux__
    // Use X11 XTest to synthesize mouse button events so it works for LWJGL/GLFW clients
    Display* display = XOpenDisplay(nullptr);
    if (display) {
        // XTest uses 1-based button numbering: 1=left, 2=middle, 3=right
        int xButton = button + 1;
        XTestFakeButtonEvent(display, xButton, down ? True : False, CurrentTime);
        XFlush(display);
        XCloseDisplay(display);
        LOG_DEBUG("[X11] Simulated mouse click: button=" + std::to_string(button) + " down=" + std::to_string(down));
        return;
    }
#endif
    // Fallback to SDL events (only effective if host app uses SDL)
    SDL_Event event;
    SDL_zero(event);
    event.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    event.button.button = button + 1; // SDL is 1-based
    event.button.state = down ? SDL_PRESSED : SDL_RELEASED;
    int x, y;
    SDL_GetMouseState(&x, &y);
    event.button.x = x;
    event.button.y = y;
    SDL_PushEvent(&event);
    LOG_DEBUG("[SDL] Simulated mouse click: button=" + std::to_string(button) + " down=" + std::to_string(down));
}

bool MinecraftClient::isAimingAtBlock() const {
    static uintptr_t aimingAtBlockAddress = 0;
    
    if (aimingAtBlockAddress == 0) {
        // Minecraft 1.8.9: objectMouseOver != null check
        // Pattern for EntityRenderer.getMouseOver() result
        aimingAtBlockAddress = memoryManager->findPattern("minecraft",
            "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 80 B8 ?? ?? ?? ?? 00",
            "xxx????xxxx?xx????x");
        if (aimingAtBlockAddress == 0) {
            // Fallback pattern for older versions
            aimingAtBlockAddress = memoryManager->findPattern("minecraft",
                "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 01",
                "xx????xxxx?xx");
        }
    }
    
    if (aimingAtBlockAddress == 0) {
        return false;
    }
    
    uintptr_t mouseOverObj = 0;
    if (memoryManager->read(aimingAtBlockAddress, &mouseOverObj, sizeof(uintptr_t))) {
        return mouseOverObj != 0;
    }
    
    return false;
}

bool MinecraftClient::isHoldingWeapon() const {
    static uintptr_t heldItemAddress = 0;
    
    if (heldItemAddress == 0) {
        // Minecraft 1.8.9: Player inventory current item
        // Pattern for EntityPlayer.inventory.currentItem
        heldItemAddress = memoryManager->findPattern("minecraft",
            "48 8B 80 ?? ?? ?? ?? 48 8B 40 ?? 48 8B 80 ?? ?? ?? ?? 48 8B 00",
            "xxx??xxx?xxx??xxx");
        if (heldItemAddress == 0) {
            // Alternative pattern for ItemStack access
            heldItemAddress = memoryManager->findPattern("minecraft",
                "8B 80 ?? ?? ?? ?? 8B 80 ?? ?? ?? ?? 89 83 ?? ?? ?? ??",
                "xx??xx??xx??xx??");
        }
    }
    
    if (heldItemAddress == 0) {
        return true;
    }
    
    // Read current held item ID
    int32_t heldItemId = 0;
    if (memoryManager->read(heldItemAddress, &heldItemId, sizeof(int32_t))) {
        // Minecraft weapon IDs: 256-279 (swords, axes, etc), 261 (bow)
        return (heldItemId >= 256 && heldItemId <= 279) || heldItemId == 261;
    }
    
    return true;
}

bool MinecraftClient::isUsingItem() const {
    static uintptr_t usingItemAddress = 0;
    
    if (usingItemAddress == 0) {
        // Minecraft 1.8.9: EntityPlayer.isUsingItem
        // Pattern for the boolean field indicating item use
        usingItemAddress = memoryManager->findPattern("minecraft",
            "80 B9 ?? ?? ?? ?? 00 74 ?? 48 8B 01 FF 90 ?? ?? ?? ??",
            "xx??xxxx?xxxx??");
        if (usingItemAddress == 0) {
            // Fallback pattern
            usingItemAddress = memoryManager->findPattern("minecraft",
                "0F B6 81 ?? ?? ?? ?? 84 C0 74 ??",
                "xx??xxxxx?x");
        }
    }
    
    if (usingItemAddress == 0) {
        return false;
    }
    
    bool usingItem = false;
    if (memoryManager->read(usingItemAddress, &usingItem, sizeof(bool))) {
        return usingItem;
    }
    
    return false;
}

bool MinecraftClient::isInventoryOpen() const {
    static uintptr_t currentScreenAddress = 0;
    
    if (currentScreenAddress == 0) {
        // Minecraft 1.8.9: Minecraft.currentScreen
        // Pattern for the GuiScreen field in Minecraft class
        currentScreenAddress = memoryManager->findPattern("minecraft",
            "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 80 ?? ?? ?? ?? 48 85 C0 74 ??",
            "xxx????xxxx?xxx????xxxx?x");
        if (currentScreenAddress == 0) {
            // Alternative pattern for GUI detection
            currentScreenAddress = memoryManager->findPattern("minecraft",
                "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 01",
                "xx????xxxx?xx");
        }
    }
    
    if (currentScreenAddress == 0) {
        return false;
    }
    
    uintptr_t currentScreen = 0;
    if (memoryManager->read(currentScreenAddress, &currentScreen, sizeof(uintptr_t))) {
        return currentScreen != 0;
    }
    
    return false;
}

std::mutex g_hookMutex;

void MinecraftClient::setup_hooks() {
    std::lock_guard<std::mutex> lock(g_hookMutex);
    LOG_INFO("Setting up hooks...");
    
    // Call the global setup_hooks function from hooks.cpp
    ::setup_hooks();
    
    LOG_SUCCESS("Hooks setup complete");
}

__attribute__((constructor))
void entry() {
    // This function is the entry point when the library is loaded into the target process.
    std::ofstream logfile;
    logfile.open("/tmp/minecraft_injector.log", std::ios_base::app);
    logfile << "[" << __TIME__ << "] Library loaded successfully! PID: " << getpid() << std::endl;
    logfile.close();
    
    // Initialize the client framework.
    MinecraftClient::getInstance().initialize();
    LOG_SUCCESS("Library entry point executed. MinecraftClient has been initialized.");
}

// Update thread implementation

void MinecraftClient::startUpdateThread() {
    if (updateThreadRunning.load()) return;
    updateThreadRunning.store(true);
    updateThread = std::thread([this]() {
        while (updateThreadRunning.load()) {
            try {
                if (moduleManager) {
                    moduleManager->onUpdate();
                }
            } catch (const std::exception &e) {
                LOG_ERROR("Update thread exception: " + std::string(e.what()));
            }
            // 20 updates per second
            Timer::sleep(50);
        }
    });
    LOG_INFO("Update thread started");
}

void MinecraftClient::stopUpdateThread() {
    if (!updateThreadRunning.load()) return;
    updateThreadRunning.store(false);
    if (updateThread.joinable()) {
        updateThread.join();
    }
    LOG_INFO("Update thread stopped");
}

__attribute__((destructor))
void cleanup() {
    // This function is called when the library is unloaded.
    LOG_INFO("Library is being unloaded, shutting down client...");
    MinecraftClient::getInstance().shutdown();
    LOG_SUCCESS("Client shutdown complete.");
}

// ... rest of the file ...