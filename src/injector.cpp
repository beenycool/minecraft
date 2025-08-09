#include "process_manager.h"
#include "logger.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <linux/limits.h>
#include <signal.h>

namespace fs = std::filesystem;

int main() {
    std::cout << "Injector starting...\n"; // Ensure we see output immediately
    LOG_INFO("=== Minecraft Injector ===");
    LOG_INFO("Debug: Entered main()");

    // Initialize core logger first
    // Logger initialized automatically via singleton instance
    LOG_INFO("Logger initialized");

    LOG_INFO("Debug: Creating ProcessManager");
    ProcessManager pm;
    
    LOG_INFO("Debug: Calling findBestMinecraftProcess()");
    ProcessInfo mcProcess = pm.findBestMinecraftProcess();

    if (mcProcess.pid == -1) {
        LOG_ERROR("Could not find a running Minecraft process.");
        LOG_INFO("Please start Minecraft (e.g., Badlion, Lunar) and try again.");
        return 1;
    }

    LOG_INFO("Found Minecraft process '" + mcProcess.name + "' (PID: " + std::to_string(mcProcess.pid) + ")");

    char self_path[PATH_MAX] = {0};
    LOG_INFO("Debug: Calling readlink()");
    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len == -1 || len >= static_cast<ssize_t>(sizeof(self_path))) {
        LOG_ERROR("Could not determine injector path");
        return 1;
    }
    self_path[len] = '\0';

    LOG_INFO("Debug: Determining libMinecraftInjectable.so path");
    fs::path libPath = fs::path(self_path).parent_path() / "libMinecraftInjectable.so";
    LOG_INFO("Debug: libPath = " + libPath.string());

    if (!fs::exists(libPath)) {
        LOG_ERROR("Could not find library: " + libPath.string());
        return 1;
    }
    
    LOG_INFO("Using library path: " + libPath.string());

    // Verify process still exists before injection
    LOG_INFO("Checking process existence PID: " + std::to_string(mcProcess.pid));
    if (kill(mcProcess.pid, 0) != 0) {
        LOG_ERROR("Process " + std::to_string(mcProcess.pid) + " no longer exists");
        return 1;
    }

    LOG_INFO("Debug: Calling injectSO");
    if (pm.injectSO(mcProcess.pid, libPath.string())) {
        LOG_SUCCESS("Injection command sent successfully.");
        Logger::flush();
        // Allow time for injection to complete before exiting
        LOG_INFO("Waiting for injection to complete...");
        usleep(500000); // Increase to 500ms delay
        // Verify process survival one last time
        if (kill(mcProcess.pid, 0) == 0) {
            LOG_SUCCESS("Injection completed successfully");
        } else {
            LOG_ERROR("Target process died during injection");
        }
        LOG_INFO("Exiting injector");
        _exit(0);
    } else {
        LOG_ERROR("Injection failed.");
        Logger::flush();
        _exit(1); // Immediate exit without static destructors
    }
}