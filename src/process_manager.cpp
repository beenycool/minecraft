#include <sys/user.h>
#include "process_manager.h"
#include "logger.h"
#include <signal.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <regex>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <cstdio> // For popen
#include <array>   // For std::array
#include <cctype>

// ... constructor, destructor, and process finders are unchanged ...

ProcessManager::ProcessManager() {}

ProcessManager::~ProcessManager() {}

std::vector<ProcessInfo> ProcessManager::getAllProcesses() {
    std::vector<ProcessInfo> processes;
    DIR* dir = opendir("/proc");
    if (!dir) {
        return processes;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            ProcessInfo info = getProcessInfo(pid);
            if (info.pid != -1) {
                processes.push_back(info);
            }
        }
    }
    closedir(dir);
    return processes;
}

std::vector<ProcessInfo> ProcessManager::getMinecraftProcesses() {
    std::vector<ProcessInfo> allProcesses = getAllProcesses();
    std::vector<ProcessInfo> minecraftProcesses;

    for (const auto& process : allProcesses) {
        if (isMinecraftProcess(process)) {
            minecraftProcesses.push_back(process);
        }
    }

    return minecraftProcesses;
}

ProcessInfo ProcessManager::findBestMinecraftProcess() {
    std::vector<ProcessInfo> minecraftProcesses = getMinecraftProcesses();

    if (minecraftProcesses.empty()) {
        return ProcessInfo{-1, "", "", false, "", ""};
    }

    if (!minecraftProcesses.empty()) {
        LOG_INFO("Found " + std::to_string(minecraftProcesses.size()) + " Minecraft Java process(es).");
        // Prioritize the first available Java process
        LOG_INFO("Selecting Minecraft process: " + minecraftProcesses[0].name + " (PID: " + std::to_string(minecraftProcesses[0].pid) + ")");
        return minecraftProcesses[0];
    }

    LOG_INFO("No suitable Minecraft process found.");
    return {};
}


bool ProcessManager::injectSO(pid_t pid, const std::string& soPath) {
    LOG_INFO("Attempting to inject library: " + soPath);
    
    LOG_INFO("Trying GDB injection method...");
    if (gdbInject(pid, soPath)) {
        LOG_SUCCESS("GDB injection command succeeded.");
        // Give the library a moment to load and run its constructor
        usleep(500000); // 500ms delay
        if (kill(pid, 0) == 0) {
            LOG_SUCCESS("Injection appears to be successful, target process is alive.");
            return true;
        } else {
             LOG_ERROR("Target process died after GDB injection.");
        }
    }

    LOG_WARN("GDB injection failed. Trying ptrace injection as a last resort...");
    try {
        inject_library(pid, soPath);
        usleep(500000);
        if (kill(pid, 0) == 0) {
            LOG_SUCCESS("Ptrace injection seems to have succeeded.");
            return true;
        }
        LOG_ERROR("Target process crashed after ptrace injection");
    } catch (const std::exception& e) {
        LOG_ERROR("Ptrace injection failed with exception: " + std::string(e.what()));
    }

    LOG_ERROR("All injection methods failed for PID " + std::to_string(pid));
    return false;
}

bool ProcessManager::gdbInject(pid_t pid, const std::string& soPath) {
    struct stat buffer;
    if (stat(soPath.c_str(), &buffer) != 0) {
        LOG_ERROR("Library file does not exist: " + soPath);
        return false;
    }

    LOG_INFO("Using GDB to inject library...");

    std::string gdb_command = "gdb -n -q -batch -p " + std::to_string(pid) +
                              " --eval-command=\"call (void*)dlopen(\\\"" + soPath + "\\\", 2)\"" + // RTLD_NOW = 2
                              " --eval-command=\"print (char*)dlerror()\"" +
                              " --eval-command=\"detach\"" +
                              " --eval-command=\"quit\"";
    
    LOG_INFO("[CMD] " + gdb_command);

    std::string output;
    std::array<char, 256> pipe_buffer;
    FILE* pipe = popen(gdb_command.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("popen() failed for GDB command!");
        return false;
    }
    while (fgets(pipe_buffer.data(), pipe_buffer.size(), pipe) != nullptr) {
        output += pipe_buffer.data();
    }
    int gdb_exit_code = pclose(pipe);

    LOG_INFO("GDB output:\n---\n" + output + "\n---");

    // Check for common failure indicators in GDB output
    if (output.find("No such process") != std::string::npos) {
        LOG_ERROR("GDB Error: Process " + std::to_string(pid) + " not found.");
        return false;
    }
    if (output.find("Operation not permitted") != std::string::npos) {
        LOG_ERROR("GDB Error: Operation not permitted. Try running the injector with 'sudo'.");
        return false;
    }

    // Normalize output for case-insensitive checks
    std::string output_lower = output;
    std::transform(output_lower.begin(), output_lower.end(), output_lower.begin(), ::tolower);

    // Detect successful dlopen pointer result ($1)
    bool dlopen_non_null = false;
    {
        std::smatch ptr_match;
        // Match both typed and untyped pointer forms
        if (std::regex_search(output, ptr_match, std::regex(R"(\$1\s*=\s*(?:\(void \*\)\s*)?(0x[0-9a-fA-F]+))"))) {
            std::string ptr = ptr_match.str(1);
            if (ptr != "0x0" && ptr != "0x00000000") {
                dlopen_non_null = true;
            }
        }
    }

    // Detect dlerror() == NULL ($2 = 0x0)
    bool dlerror_null = (output.find("$2 = 0x0") != std::string::npos);

    // Detect detach success messages printed by gdb in various formats
    bool has_detached = (output.find("Detaching from process") != std::string::npos) ||
                        (output_lower.find("detached]") != std::string::npos) ||
                        (output_lower.find("inferior 1 (process") != std::string::npos && output_lower.find("detached") != std::string::npos);

    // Detect explicit dlopen NULL as failure regardless of exit code
    bool dlopen_null = false;
    {
        std::smatch null_match;
        if (std::regex_search(output, null_match, std::regex(R"(\$1\s*=\s*(?:\(void \*\)\s*)?(0x0+))"))) {
            dlopen_null = true;
        }
    }
    if (dlopen_null && !dlerror_null) {
        LOG_ERROR("dlopen() returned NULL and dlerror() is not NULL. Injection failed.");
        return false;
    }

    // Consider success if gdb exited cleanly AND we have any strong success signal
    if (gdb_exit_code == 0 && (dlopen_non_null || dlerror_null || has_detached)) {
        LOG_SUCCESS("GDB indicates success (dlopen non-null or dlerror NULL or detached)." );
        return true;
    }

    LOG_WARN("GDB injection finished with an ambiguous result. Exit code: " + std::to_string(gdb_exit_code));
    return false;
}

// ... the rest of the file (getProcessInfo, etc.) remains the same ...

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <elf.h>
#include <link.h>

void ProcessManager::inject_library(pid_t pid, const std::string& lib_path) {
    throw std::runtime_error("Ptrace injection is not fully implemented and is unsafe to use.");
    // ... original ptrace code that was here is omitted for safety ...
}

ProcessInfo ProcessManager::getProcessInfo(pid_t pid) {
    ProcessInfo info;
    info.pid = pid;

    // Read process name from /proc/[pid]/status
    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream status_file(status_path);
    if (status_file.is_open()) {
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.find("Name:") == 0) {
                // Extract the name
                info.name = line.substr(6); // Skip "Name:"
                // Remove leading/trailing whitespace
                size_t start = info.name.find_first_not_of(" \t");
                size_t end = info.name.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    info.name = info.name.substr(start, end - start + 1);
                }
                break;
            }
        }
        status_file.close();
    }

    // Read command line from /proc/[pid]/cmdline
    std::string cmdline_path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream cmdline_file(cmdline_path);
    if (cmdline_file.is_open()) {
        std::string cmdline;
        std::getline(cmdline_file, cmdline);
        // The cmdline is a null-separated string, replace nulls with spaces
        std::replace(cmdline.begin(), cmdline.end(), '\0', ' ');
        info.cmdline = cmdline;
        cmdline_file.close();
    }

    // Initially set to false; we'll set it later in isMinecraftProcess
    info.isMinecraft = false;
    info.clientType = "";
    info.version = "";

    // If we couldn't read the name, set pid to -1 to indicate failure
    if (info.name.empty()) {
        info.pid = -1;
    }

    return info;
}

bool ProcessManager::isMinecraftProcess(const ProcessInfo& process) {
    // Check if the process name is "java"
    if (process.name != "java") {
        return false;
    }

    // Check the command line for Minecraft indicators
    if (process.cmdline.find("net.minecraft") != std::string::npos ||
        process.cmdline.find("minecraft") != std::string::npos) {
        return true;
    }

    return false;
}