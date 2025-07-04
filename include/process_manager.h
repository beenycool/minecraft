#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <sys/types.h>

struct ProcessInfo {
    pid_t pid;
    std::string name;
    std::string cmdline;
    bool isMinecraft;
    std::string clientType; // "vanilla", "forge", "lunar", "badlion", "optifine"
    std::string version;
};

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();
    
    std::vector<ProcessInfo> getMinecraftProcesses();
    ProcessInfo findBestMinecraftProcess();
    bool injectSO(pid_t pid, const std::string& soPath);
    pid_t findProcessByName(const std::string& processName);
    
private:
    bool gdbInject(pid_t pid, const std::string& soPath);
    std::string findLibcPath(pid_t pid);
    bool gdbInjectAlternative(pid_t pid, const std::string& soPath);
    bool gdbInjectFallback(pid_t pid, const std::string& soPath);

private:
    std::vector<ProcessInfo> getAllProcesses();
    ProcessInfo getProcessInfo(pid_t pid);
    bool isMinecraftProcess(const ProcessInfo& process);
    std::string detectClientType(const ProcessInfo& process);
    std::string detectGameVersion(const ProcessInfo& process);

    // Injection logic
    void inject_library(pid_t pid, const std::string& lib_path);
    uintptr_t find_library_base(pid_t pid, const char* lib_name);
    void* get_remote_func_addr(pid_t pid, const char* lib_name, const void* local_func_addr);
    void write_to_memory(pid_t pid, uintptr_t addr, const void* data, size_t size);
    long call_remote_function(pid_t pid, uintptr_t func_addr, const std::vector<long>& args);
};