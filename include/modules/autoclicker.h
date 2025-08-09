#pragma once

#include "../module_manager.h"
#include "../timer.h"

class AutoClicker : public Module {
public:
    AutoClicker();
    void onEnable() override;
    void onDisable() override;
    void onUpdate() override;
    void onMouseButton(int button, int action) override;
    void onKey(int key, int action) override;
    void onRender() override;

private:
    void loadConfig();
    
    enum ClickPattern {
        JITTER,
        BUTTERFLY
    };

    bool isLeftMouseDown = false;
    bool inInventory = false;
    bool shiftKeyDown = false;
    int consecutiveClicks = 0;
    double nextClickTime = 0.0;

    ClickPattern clickPattern = JITTER;
    int targetCPS = 10;
    bool randomize = true;
    float randomizeMean = 1.0f;
    float randomizeStdDev = 0.1f;
    bool simulateExhaust = true;
    bool allowBreakingBlocks = true;
    bool allowInventory = true;
    bool randomizeInventorySpeed = true;
    bool holdingWeapon = false;
    bool notUsingItem = false;

    Timer clickTimer;
    
    bool checkConditionals();
    void performClick();
    void performDoubleClick();
    double calculateNextDelay();
    void updateConditionals();
};