#include "memory_manager.h"
uintptr_t MemoryManager::getSymbolAddress(const std::string&) { return 0x12345678; }
MemoryManager* MemoryManager::instance = nullptr;
#include "memory_manager.h"
#include "logger.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <sys/uio.h>
#include <iomanip>
#include <cerrno>

MemoryManager::MemoryManager(pid_t pid) : pid(pid) {}

bool MemoryManager::read(uintptr_t address, void* buffer, size_t size) {
    // If pid is 0, we're in the same process, use direct memory access
    if (pid == 0) {
        // Direct memory copy when we're in the same process
        try {
            memcpy(buffer, (void*)address, size);
            return true;
        } catch (...) {
            LOG_ERROR("Failed to read " + std::to_string(size) + " bytes from 0x" +
                      std::to_string(address) + " (direct access)");
            return false;
        }
    }
    
    // Otherwise use process_vm_readv for external process
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = size;

    ssize_t totalRead = 0;
    ssize_t nread;
    int attempts = 0;
    const int maxAttempts = 3;
    uint8_t* current = static_cast<uint8_t*>(buffer);
    
    while (totalRead < (ssize_t)size && attempts < maxAttempts) {
        nread = process_vm_readv(pid, local, 1, remote, 1, 0);
        if (nread < 0) {
            int err = errno;
            LOG_DEBUG("process_vm_readv error: " + std::string(strerror(err)) +
                      " (" + std::to_string(err) + ") at 0x" + std::to_string(address));
            attempts++;
            continue;
        }
        
        totalRead += nread;
        if (totalRead < (ssize_t)size) {
            // Adjust pointers for partial read
            current += nread;
            local[0].iov_base = current;
            local[0].iov_len = size - totalRead;
            remote[0].iov_base = (void*)(address + totalRead);
            remote[0].iov_len = size - totalRead;
        }
    }
    
    if (totalRead != (ssize_t)size) {
        LOG_ERROR("Failed to read " + std::to_string(size) + " bytes from 0x" +
                  std::to_string(address) + " after " + std::to_string(maxAttempts) + " attempts");
        return false;
    }
    return true;
}

bool MemoryManager::write(uintptr_t address, void* buffer, size_t size) {
    // If pid is 0, we're in the same process, use direct memory access
    if (pid == 0) {
        // Direct memory copy when we're in the same process
        try {
            memcpy((void*)address, buffer, size);
            return true;
        } catch (...) {
            LOG_ERROR("Failed to write " + std::to_string(size) + " bytes to 0x" +
                      std::to_string(address) + " (direct access)");
            return false;
        }
    }
    
    // Otherwise use process_vm_writev for external process
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = size;

    ssize_t nwritten = process_vm_writev(pid, local, 1, remote, 1, 0);
    if (nwritten != (ssize_t)size) {
        LOG_ERROR("Failed to write " + std::to_string(size) + " bytes to 0x" + std::to_string(address));
        return false;
    }
    return true;
}

uintptr_t MemoryManager::patternScan(uintptr_t start, size_t length, const std::string& pattern_str, const std::string& mask) {
    std::vector<unsigned char> pattern;
    std::stringstream ss(pattern_str);
    std::string byte_str;
    while (ss >> byte_str) {
        pattern.push_back(static_cast<unsigned char>(std::stoul(byte_str, nullptr, 16)));
    }

    std::vector<unsigned char> data(length);
    if (!read(start, data.data(), length)) {
        LOG_ERROR("patternScan: Failed to read memory at 0x" + std::to_string(start));
        return 0;
    }

    for (size_t i = 0; i < length - pattern.size(); ++i) {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); ++j) {
            if (mask[j] != '?' && pattern[j] != data[i + j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return start + i;
        }
    }
    return 0;
}

uintptr_t MemoryManager::getModuleBase(const std::string& moduleName) {
    for (const auto& module : getModules()) {
        if (module.name.find(moduleName) != std::string::npos) {
            return module.base;
        }
    }
    return 0;
}



uintptr_t MemoryManager::findPattern(const std::string& moduleName, const std::string& pattern, const std::string& mask) {
    LOG_DEBUG("Pattern scan for: " + pattern + " in module: " + moduleName);
    for (const auto& module : getModules()) {
        if (moduleName.empty() || module.name.find(moduleName) != std::string::npos) {
            LOG_DEBUG("Scanning region: 0x" + std::to_string(module.base) + "-0x" + std::to_string(module.end) + " (" + module.name + ")");
            uintptr_t result = patternScan(module.base, module.end - module.base, pattern, mask);
            if (result != 0) {
                return result;
            }
        }
    }
    return 0;
}



std::vector<MemoryManager::ModuleInfo> MemoryManager::getModules() {
    std::vector<ModuleInfo> modules;
    char maps_path[256];
    
    // If pid is 0, use self for current process
    if (pid == 0) {
        sprintf(maps_path, "/proc/self/maps");
    } else {
        sprintf(maps_path, "/proc/%d/maps", pid);
    }

    std::ifstream maps_file(maps_path);
    if (!maps_file.is_open()) {
        LOG_ERROR("Failed to open " + std::string(maps_path));
        return modules;
    }

    std::string line;
    while (std::getline(maps_file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string address_range, perms, offset, dev, inode, path;
        
        iss >> address_range >> perms >> offset >> dev >> inode;
        std::getline(iss, path);

        // Trim leading spaces from path
        if (!path.empty()) {
            size_t first = path.find_first_not_of(" \t");
            if (std::string::npos != first) {
                path = path.substr(first);
            }
        }

        if (perms.find('x') == std::string::npos) {
            continue;
        }

        size_t dash_pos = address_range.find('-');
        if (dash_pos == std::string::npos) continue;
        uintptr_t start = std::stoull(address_range.substr(0, dash_pos), nullptr, 16);
        uintptr_t end = std::stoull(address_range.substr(dash_pos + 1), nullptr, 16);
        if (end <= start) continue;

        modules.push_back({path, start, end});
    }

    return modules;
}