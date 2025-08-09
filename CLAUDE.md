# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C++ injection framework for Minecraft 1.8.9 clients (Badlion, Lunar, Forge, Vanilla). It uses dynamic library injection to add gameplay modifications through a modular system.

## Build Commands

```bash
# Initial setup
mkdir -p build
cd build
cmake ..

# Build the project
make -j$(nproc)

# This creates:
# - libMinecraftInjectable.so (injectable library)
# - injector (injection executable)
```

## Testing and Running

```bash
# Test injection (Minecraft must be running)
./test_injection.sh

# Run injector
./build/injector

# Monitor logs
tail -f /tmp/minecraft_injectable.log
tail -f logs/minecraft_injectable.log
```

## Architecture

### Core Components

1. **MinecraftClient** (client.h/cpp) - Central singleton managing the framework
   - Initializes all managers
   - Handles module lifecycle
   - Manages global state

2. **Managers**
   - **ProcessManager**: Finds Minecraft processes, performs injection
   - **MemoryManager**: Read/write game memory, pattern scanning
   - **HookManager**: SDL and function hooking infrastructure
   - **ModuleManager**: Loads and manages feature modules

3. **Module System**
   - Base class: `Module` (include/modules/module.h)
   - Modules inherit and implement: `onEnable()`, `onDisable()`, `onTick()`
   - Located in: `src/modules/` and `include/modules/`
   - Configured via: `config/modules/*.json`

4. **Hook System**
   - SDL2 event hooks for keyboard/mouse input
   - Function detours for game functions
   - Event distribution to modules

### Key Directories

- `src/` - Implementation files
- `include/` - Headers
- `src/modules/` - Module implementations
- `config/` - JSON configuration files
- `build/` - CMake build output

### Adding New Modules

1. Create header in `include/modules/`
2. Implement in `src/modules/`
3. Register in `ModuleManager::loadModules()`
4. Add config in `config/modules/` if needed

### Configuration

- Main config: `config/config.json`
- Module configs: `config/modules/*.json`
- Keybinds, injection settings, anti-detection options

### Dependencies

- CMake 3.15+
- C++17 compiler
- SDL2 (for input hooking)
- nlohmann/json (fetched automatically)

## Important Notes

- This is a game modification framework using injection techniques
- Requires appropriate permissions to inject into processes
- Logs are written to `/tmp/` and `logs/` directories
- Module keybinds: F1 (Speed), F2 (Flight), F3 (ESP), F4 (AutoClicker), F5 (Fullbright)