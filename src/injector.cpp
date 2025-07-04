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

    // Make the log file world-writable so the injected library can write to it
    try {
        fs::permissions("/tmp/minecraft_injectable.log",
                        fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all,
                        fs::perm_options::replace);
        LOG_INFO("Debug: Set log file permissions");
    } catch (const std::exception& e) {
        std::cerr << "Permission error: " << e.what() << "\n";
        LOG_ERROR("Debug: Exception setting log file permissions: " + std::string(e.what()));
        return 1;
    }

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
    fs::path libPathOriginal = fs::path(self_path).parent_path() / "libMinecraftInjectable.so";
    LOG_INFO("Debug: libPathOriginal = " + libPathOriginal.string());

    if (!fs::exists(libPathOriginal)) {
        LOG_ERROR("Could not find library: " + libPathOriginal.string());
        return 1;
    }

    // To avoid issues with spaces or special characters in the build directory path,
    // copy the library to /tmp (which is almost always free of such characters) and
    // inject it from there.
    fs::path tempLibPath = "/tmp/libMinecraftInjectable.so";
    try {
        fs::copy_file(libPathOriginal, tempLibPath, fs::copy_options::overwrite_existing);
        // Ensure library has execute permissions
        fs::permissions(tempLibPath,
                        fs::perms::owner_all | fs::perms::group_exec | fs::perms::others_exec,
                        fs::perm_options::replace);
        LOG_INFO("Copied and set permissions for " + tempLibPath.string());
    } catch (const fs::filesystem_error& e) {
        LOG_WARN("Could not copy library to /tmp (" + std::string(e.what()) + "). Falling back to original path.");
        tempLibPath = libPathOriginal; // fall back
    }

    // Verify process still exists before injection
    LOG_INFO("Checking process existence PID: " + std::to_string(mcProcess.pid));
    if (kill(mcProcess.pid, 0) != 0) {
        LOG_ERROR("Process " + std::to_string(mcProcess.pid) + " no longer exists");
        return 1;
    }

    LOG_INFO("Debug: Calling injectSO");
    if (pm.injectSO(mcProcess.pid, tempLibPath.string())) {
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