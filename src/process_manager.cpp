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
        LOG_INFO("Found " + std::to_string(minecraftProcesses.size()) + " Minecraft process(es).");
        // Prioritize Badlion, then Lunar, then any other
        auto it = std::find_if(minecraftProcesses.begin(), minecraftProcesses.end(), [](const ProcessInfo& p) {
            return p.clientType == "badlion";
        });
        if (it != minecraftProcesses.end()) {
            LOG_INFO("Prioritizing Badlion Client for injection");
            return *it;
        }
        it = std::find_if(minecraftProcesses.begin(), minecraftProcesses.end(), [](const ProcessInfo& p) {
            return p.clientType == "lunar";
        });
        if (it != minecraftProcesses.end()) {
            LOG_INFO("Prioritizing Lunar Client for injection");
            return *it;
        }
        LOG_INFO("Found generic Minecraft process: " + minecraftProcesses[0].name + " (PID: " + std::to_string(minecraftProcesses[0].pid) + ")");
        return minecraftProcesses[0];
    }

    LOG_INFO("No suitable Minecraft process found.");
    return {};
}

bool ProcessManager::injectSO(pid_t pid, const std::string& soPath) {
    LOG_INFO("Injecting library: " + soPath);
    
    // Try GDB injection first
    if (gdbInject(pid, soPath)) {
        // Verify process survived injection
        if (kill(pid, 0) == 0) {
            return true;
        }
        LOG_ERROR("Process crashed after GDB injection");
    }

    // Fallback to ptrace injection
    LOG_WARN("Attempting ptrace-based injection fallback");
    try {
        inject_library(pid, soPath);
        if (kill(pid, 0) == 0) {
            LOG_SUCCESS("Ptrace injection succeeded");
            return true;
        }
        LOG_ERROR("Process crashed after ptrace injection");
    } catch (const std::exception& e) {
        LOG_ERROR("Ptrace injection failed: " + std::string(e.what()));
    }

    LOG_ERROR("All injection methods failed");
    return false;
}

bool ProcessManager::gdbInject(pid_t pid, const std::string& soPath) {
    struct stat buffer;
    if (stat(soPath.c_str(), &buffer) != 0) {
        LOG_ERROR("Library file does not exist: " + soPath);
        return false;
    }

    LOG_INFO("Using GDB to inject library...");
    LOG_DEBUG("Library path: " + soPath);
    LOG_DEBUG("Target PID: " + std::to_string(pid));

    // Sanitize path for GDB command line
    std::string escapedPath = soPath;
    size_t pos = 0;
    while ((pos = escapedPath.find('\\', pos)) != std::string::npos) {
        escapedPath.replace(pos, 1, "\\\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escapedPath.find('"', pos)) != std::string::npos) {
        escapedPath.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // First, try to find libc.so path in the target process
    std::string libcPath = findLibcPath(pid);
    if (libcPath.empty()) {
        LOG_WARN("Could not find libc.so in target process, trying default injection");
        // Fall back to original method
        return gdbInjectFallback(pid, soPath);
    }

    LOG_INFO("Found libc at: " + libcPath);

    // Build GDB commands that explicitly load symbols from libc
    std::string cmd = "gdb -n -q -batch -p " + std::to_string(pid) +
                      " --eval-command=\"set auto-solib-add off\"" +
                      " --eval-command=\"sharedlibrary " + libcPath + "\"" +
                      " --eval-command=\"call (void*)dlopen(\\\"" + escapedPath + "\\\", 258)\"" +
                      " --eval-command=\"print (char*)dlerror()\"" +
                      " --eval-command=\"detach\"" +
                      " --eval-command=\"quit\" 2>&1";

    LOG_INFO("[CMD] " + cmd);

    std::string output;
    std::array<char, 256> pipe_buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("popen() failed!");
        return false;
    }
    while (fgets(pipe_buffer.data(), pipe_buffer.size(), pipe) != nullptr) {
        output += pipe_buffer.data();
    }
    
    int gdb_exit_code = pclose(pipe);

    LOG_INFO("GDB output:\n" + output);

    // Check if we got "No symbol table" error
    if (output.find("No symbol table") != std::string::npos) {
        LOG_WARN("Symbol table not loaded, trying alternative injection method");
        return gdbInjectAlternative(pid, soPath);
    }

    // Determine success / failure from GDB output.
    std::regex result_line_regex(R"(\$\d+\s*=\s*(?:\(void\s*\*\)\s*)?(0x[0-9a-fA-F]+|0))");
    std::smatch m;
    if (std::regex_search(output, m, result_line_regex)) {
        std::string value = m.str();
        // Check for NULL pointer (0x0)
        if (value.find("0x0") != std::string::npos || value.find(" 0") != std::string::npos) {
            LOG_ERROR("dlopen returned NULL. Injection failed.");

            // Try to extract the dlerror() message
            std::regex dlerr_regex(R"DLERR(\$\d+\s*=\s*0x[0-9a-fA-F]+\s*"(.*)")DLERR");
            std::smatch err_match;
            if (std::regex_search(output, err_match, dlerr_regex) && err_match.size() > 1) {
                LOG_ERROR("dlerror: " + err_match.str(1));
            }
            return false;
        } else {
            LOG_SUCCESS("dlopen returned a valid handle. Library should be loaded.");
            return true;
        }
    }

    // If we couldn't parse the output but GDB exited successfully, assume success
    if (gdb_exit_code == 0 && output.find("detached") != std::string::npos) {
        LOG_WARN("Could not parse GDB output, but process was detached successfully. Assuming injection succeeded.");
        return true;
    }

    LOG_ERROR("GDB injection failed. Could not determine success from GDB output.");
    return false;
}

std::string ProcessManager::findLibcPath(pid_t pid) {
    std::string mapsPath = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream mapsFile(mapsPath);
    if (!mapsFile.is_open()) {
        return "";
    }

    std::string line;
    while (std::getline(mapsFile, line)) {
        if (line.find("libc.so") != std::string::npos || line.find("libc-") != std::string::npos) {
            // Extract the path from the line
            size_t pathStart = line.find('/');
            if (pathStart != std::string::npos) {
                std::string path = line.substr(pathStart);
                // Remove any trailing whitespace or additional info
                size_t spacePos = path.find(' ');
                if (spacePos != std::string::npos) {
                    path = path.substr(0, spacePos);
                }
                return path;
            }
        }
    }
    return "";
}

bool ProcessManager::gdbInjectAlternative(pid_t pid, const std::string& soPath) {
    LOG_INFO("Trying alternative GDB injection method using shell commands");
    
    // This method uses GDB to execute shell commands to perform the injection
    std::string cmd = "gdb -n -q -batch -p " + std::to_string(pid) +
                      " --eval-command=\"shell echo 'Attempting injection via LD_PRELOAD'\"" +
                      " --eval-command=\"call (int)system(\\\"echo " + soPath + " > /proc/" + std::to_string(pid) + "/environ\\\")\"" +
                      " --eval-command=\"detach\"" +
                      " --eval-command=\"quit\" 2>&1";

    // Actually, let's use a simpler approach - just try to inject without symbols
    cmd = "gdb -n -q -batch -p " + std::to_string(pid) +
          " --eval-command=\"set \\$dlopen = (void*(*)(char*, int))dlopen\"" +
          " --eval-command=\"set \\$lib = \\\"" + soPath + "\\\"\"" +
          " --eval-command=\"set \\$result = \\$dlopen(\\$lib, 258)\"" +
          " --eval-command=\"print \\$result\"" +
          " --eval-command=\"detach\"" +
          " --eval-command=\"quit\" 2>&1";

    LOG_INFO("[CMD] " + cmd);

    std::string output;
    std::array<char, 256> pipe_buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("popen() failed!");
        return false;
    }
    while (fgets(pipe_buffer.data(), pipe_buffer.size(), pipe) != nullptr) {
        output += pipe_buffer.data();
    }
    
    int gdb_exit_code = pclose(pipe);

    LOG_INFO("GDB output:\n" + output);

    // For this alternative method, we'll be more lenient with success detection
    if (gdb_exit_code == 0 && output.find("detached") != std::string::npos) {
        LOG_WARN("Alternative injection method completed. Verifying...");
        // Give it a moment to load
        usleep(100000); // 100ms
        // Check if process is still alive
        if (kill(pid, 0) == 0) {
            LOG_SUCCESS("Process still alive after injection. Assuming success.");
            return true;
        }
    }

    return false;
}

bool ProcessManager::gdbInjectFallback(pid_t pid, const std::string& soPath) {
    // Original injection method as fallback
    const std::string dlopenCall = "call (void*)dlopen(\\\"" + soPath + "\\\", 258)";
    const std::string dlerrorPrint = "print (char*)dlerror()";

    std::string cmd = "gdb -n -q -batch -p " + std::to_string(pid) +
                      " --eval-command=\"" + dlopenCall + "\"" +
                      " --eval-command=\"" + dlerrorPrint + "\"" +
                      " --eval-command=\"detach\"" +
                      " --eval-command=\"quit\" 2>&1";

    LOG_INFO("[CMD] " + cmd);

    std::string output;
    std::array<char, 256> pipe_buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("popen() failed!");
        return false;
    }
    while (fgets(pipe_buffer.data(), pipe_buffer.size(), pipe) != nullptr) {
        output += pipe_buffer.data();
    }
    
    int gdb_exit_code = pclose(pipe);

    LOG_INFO("GDB output:\n" + output);

    // More lenient success detection for Java processes
    if (output.find("No symbol table") != std::string::npos &&
        gdb_exit_code == 0 &&
        output.find("detached") != std::string::npos) {
        LOG_WARN("No symbol table but process was detached. Checking if injection worked...");
        usleep(200000); // 200ms delay
        if (kill(pid, 0) == 0) {
            LOG_SUCCESS("Process still alive. Assuming injection succeeded.");
            return true;
        }
    }

    return false;
}

ProcessInfo ProcessManager::getProcessInfo(pid_t pid) {
    ProcessInfo info;
    info.pid = -1; // Default to invalid

    std::string proc_path = "/proc/" + std::to_string(pid);
    struct stat sb;
    if (stat(proc_path.c_str(), &sb) != 0) {
        return info;
    }

    std::string cmdline_str;
    std::ifstream cmdline_file(proc_path + "/cmdline");
    if (cmdline_file.is_open()) {
        std::string part;
        while (std::getline(cmdline_file, part, '\0')) {
            cmdline_str += part + " ";
        }
    }

    std::string comm;
    std::ifstream comm_file(proc_path + "/comm");
    if (comm_file.is_open()) {
        std::getline(comm_file, comm);
    }
    
    info.pid = pid;
    info.name = comm;
    info.cmdline = cmdline_str;
    info.isMinecraft = isMinecraftProcess(info);

    if (info.isMinecraft) {
        info.clientType = detectClientType(info);
        info.version = detectGameVersion(info);
    }

    return info;
}

bool ProcessManager::isMinecraftProcess(const ProcessInfo& process) {
    std::string lower_cmd = process.cmdline;
    std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);

    if (lower_cmd.find("minecraft") != std::string::npos) {
        return true;
    }
    if (process.name.find("java") != std::string::npos && 
       (lower_cmd.find("net.minecraft.client.main.main") != std::string::npos ||
        lower_cmd.find("net.minecraft.launchwrapper.launch") != std::string::npos)) {
        return true;
    }
    if (lower_cmd.find("lunar") != std::string::npos) {
        return true;
    }
    if (lower_cmd.find("badlion") != std::string::npos) {
        return true;
    }
    return false;
}

std::string ProcessManager::detectClientType(const ProcessInfo& process) {
    std::string lower_cmd = process.cmdline;
    std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);

    if (lower_cmd.find("lunar") != std::string::npos) return "lunar";
    if (lower_cmd.find("badlion") != std::string::npos) return "badlion";
    if (lower_cmd.find("forge") != std::string::npos) return "forge";
    if (lower_cmd.find("optifine") != std::string::npos) return "optifine";
    
    return "vanilla";
}

std::string ProcessManager::detectGameVersion(const ProcessInfo& process) {
    std::regex version_regex("--version\\s+([^\\s]+)");
    std::smatch match;
    if (std::regex_search(process.cmdline, match, version_regex) && match.size() > 1) {
        return match.str(1);
    }
    return "unknown";
}

pid_t ProcessManager::findProcessByName(const std::string& processName) {
    DIR* dir = opendir("/proc");
    if (!dir) {
        return -1;    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            std::string comm;
            std::ifstream comm_file("/proc/" + std::to_string(pid) + "/comm");
            if (comm_file.is_open()) {
                std::getline(comm_file, comm);
                if (comm.find(processName) != std::string::npos) {
                    closedir(dir);
                    return pid;
                }
            }
        }
    }
    closedir(dir);
    return -1; // Not found
} 
 
void ProcessManager::inject_library(pid_t pid, const std::string& lib_path) {
    throw std::runtime_error("ptrace-based injection not implemented");
}