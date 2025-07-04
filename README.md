# Minecraft Injectable Client Framework

A cross-platform injection framework for Minecraft 1.8.9 clients (Badlion, Lunar, Forge, Vanilla).

## Features
- Process detection and injection
- Memory manipulation
- Function hooking
- Modular cheat system
- Cross-platform support (Windows/Linux)

## Build Instructions

### Windows
```bash
build.bat
```

### Linux
```bash
chmod +x build.sh
./build.sh
```

## Usage

### Injection
```bash
# Windows
injector.exe

# Linux
sudo ./minecraft_inject.sh
```

## Modules
1. **Speed** (F1) - Increases movement speed
2. **Flight** (F2) - Fly in survival mode
3. **ESP** (F3) - See entities through walls
4. **AutoClicker** (F4) - Automatic clicking
5. **Fullbright** (F5) - Full brightness

## Configuration
Edit [`config/config.json`](config/config.json) to customize:
- Auto-injection settings
- Keybinds
- Module defaults
- Security features

## Project Structure
```
├── build/             # Build artifacts
├── config/            # Configuration files
├── include/           # Header files
│   ├── client.h
│   ├── hook_manager.h
│   ├── memory_manager.h
│   ├── module_manager.h
│   ├── process_manager.h
│   └── modules/       # Example modules
├── src/               # Source files
├── build.bat          # Windows build script
├── build.sh           # Linux build script
├── CMakeLists.txt     # Build configuration
├── minecraft_inject.sh # Linux injection script
└── ptrace_injector.cpp # Linux injection implementation
```

## Requirements
- CMake 3.20+
- GCC/Clang (Linux) or Visual Studio Build Tools (Windows)