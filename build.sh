#!/bin/bash

# Simple build script for SDR Network Interface System

echo "Building SDR Network Interface System..."

# Create build directory
mkdir -p build
cd build

# Run CMake to configure the project
echo "Configuring with CMake..."
cmake ..

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building..."
make -j$(nproc 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build completed successfully!"
echo "Executable location: build/bin/sdr_network"
echo ""
echo "To run (requires root privileges on Linux):"
echo "  sudo ./bin/sdr_network"
echo ""
echo "For help:"
echo "  ./bin/sdr_network --help"
