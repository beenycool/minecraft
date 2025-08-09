#!/bin/bash

echo "=== Building Minecraft Injectable Client ==="

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build the project
echo "Building the project..."
make -j$(nproc)

echo "Build completed!"
echo "Generated files:"
ls -la

cd ..