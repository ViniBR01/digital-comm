# SDR Network Interface System

A Software Defined Radio (SDR) educational project that creates a wireless communication channel between two virtual network interfaces using the Ettus UHD B210 device.

## Overview

This project implements a loopback communication system using Software Defined Radio. It creates two virtual network interfaces (TUN/TAP) in Linux that communicate with each other through the B210 SDR hardware, implementing simple modulation schemes and channel access mechanisms.

The system is designed for educational purposes, with comprehensive telemetry and visualization tools to help users understand the entire communication chain.

## Current Implementation Status

The current implementation includes:
- **Complete TUN/TAP network interface implementation** in C++
- **Packet forwarding between two virtual interfaces** (simulating wireless transmission)
- **Statistics collection and reporting**
- **Command-line interface** with configurable parameters
- **Cross-platform build system** (Linux for full functionality, macOS for development)
- **Ready for SDR hardware integration** in future phases

This initial implementation provides a solid foundation for the SDR communication system and can be tested immediately with standard networking tools.

## Requirements

- Linux operating system
- Ettus UHD B210 SDR device
- UHD driver
- Modern C++ compiler with C++17 support
- CMake 3.14 or newer
- Boost libraries

## Building from Source

```bash
# Create build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make -j$(nproc)
```

## Usage

```bash
# Basic usage (requires root privileges for TUN/TAP interfaces)
sudo ./bin/sdr_network

# Testing the network interfaces
# In terminal 1 (after starting the application):
ping 192.168.20.1 -I 192.168.10.1

# In terminal 2:
sudo iperf3 -s -B 192.168.20.1

# In terminal 3:
sudo iperf3 -c 192.168.20.1 -B 192.168.10.1
```

## Project Structure

```
sdr-network/
├── include/              # Public header files
│   └── sdr-network/      # Main project headers
├── src/                  # Implementation files
├── tests/                # Test files
├── examples/             # Example applications
├── docs/                 # Documentation
└── CMakeLists.txt        # Main CMake configuration
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.
