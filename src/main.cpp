#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include <boost/program_options.hpp>

#include "sdr-network/network/tun_tap_interface.h"

namespace po = boost::program_options;
using namespace sdr_network;

// Global flag for graceful shutdown
std::atomic<bool> running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

void printPacketInfo(const std::string& interface_name, const std::vector<uint8_t>& packet) {
    std::cout << "[" << interface_name << "] Received packet of " << packet.size() << " bytes" << std::endl;
    
    // Print first few bytes for debugging (in hex)
    std::cout << "  Data: ";
    for (size_t i = 0; i < std::min(packet.size(), size_t(16)); ++i) {
        printf("%02x ", packet[i]);
    }
    if (packet.size() > 16) {
        std::cout << "...";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command line options
    po::options_description desc("SDR Network Interface Options");
    desc.add_options()
        ("help,h", "Print help message")
        ("tun", "Use TUN interfaces instead of TAP")
        ("interface1", po::value<std::string>()->default_value("sdr_tap0"), "Name of first interface")
        ("interface2", po::value<std::string>()->default_value("sdr_tap1"), "Name of second interface")
        ("ip1", po::value<std::string>()->default_value("192.168.10.1"), "IP address for first interface")
        ("ip2", po::value<std::string>()->default_value("192.168.20.1"), "IP address for second interface")
        ("netmask", po::value<std::string>()->default_value("255.255.255.0"), "Netmask for both interfaces");
        
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }
    
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    
    // Set up signal handling
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    std::cout << "SDR Network Interface System" << std::endl;
    std::cout << "============================" << std::endl;
    
    // Determine interface type
    auto interface_type = vm.count("tun") ? 
        network::TunTapInterface::Type::TUN : 
        network::TunTapInterface::Type::TAP;
    
    std::string type_str = (interface_type == network::TunTapInterface::Type::TUN) ? "TUN" : "TAP";
    std::cout << "Using " << type_str << " interfaces" << std::endl;
    
    try {
        // Create network interfaces
        network::TunTapInterface interface1(vm["interface1"].as<std::string>(), interface_type);
        network::TunTapInterface interface2(vm["interface2"].as<std::string>(), interface_type);
        
        std::cout << "\nInitializing interfaces..." << std::endl;
        
        // Initialize interfaces
        if (!interface1.initialize()) {
            std::cerr << "Failed to initialize first interface" << std::endl;
            return 1;
        }
        
        if (!interface2.initialize()) {
            std::cerr << "Failed to initialize second interface" << std::endl;
            return 1;
        }
        
        std::cout << "Configuring interfaces..." << std::endl;
        
        // Configure interfaces with IP addresses
        std::string netmask = vm["netmask"].as<std::string>();
        if (!interface1.configure(vm["ip1"].as<std::string>(), netmask)) {
            std::cerr << "Failed to configure first interface" << std::endl;
            return 1;
        }
        
        if (!interface2.configure(vm["ip2"].as<std::string>(), netmask)) {
            std::cerr << "Failed to configure second interface" << std::endl;
            return 1;
        }
        
        std::cout << "Starting packet capture..." << std::endl;
        
        // Set up packet handlers for simple loopback
        auto handler1 = [&interface2](const std::vector<uint8_t>& packet) {
            printPacketInfo("Interface1", packet);
            // Forward packet to interface2 (simulating wireless transmission)
            interface2.writePacket(packet);
        };
        
        auto handler2 = [&interface1](const std::vector<uint8_t>& packet) {
            printPacketInfo("Interface2", packet);
            // Forward packet to interface1 (simulating wireless transmission)
            interface1.writePacket(packet);
        };
        
        // Start packet capture on both interfaces
        if (!interface1.startCapture(handler1)) {
            std::cerr << "Failed to start capture on first interface" << std::endl;
            return 1;
        }
        
        if (!interface2.startCapture(handler2)) {
            std::cerr << "Failed to start capture on second interface" << std::endl;
            return 1;
        }
        
        std::cout << "\nSystem is running. Try the following commands to test:" << std::endl;
        std::cout << "  ping " << vm["ip2"].as<std::string>() << " -I " << vm["ip1"].as<std::string>() << std::endl;
        std::cout << "  Or use iperf3 for throughput testing." << std::endl;
        std::cout << "Press Ctrl+C to stop.\n" << std::endl;
        
        // Statistics reporting thread
        std::thread stats_thread([&interface1, &interface2]() {
            while (running) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                if (!running) break;
                
                std::cout << "\n--- Statistics ---" << std::endl;
                std::cout << interface1.getName() << ": " 
                          << "RX=" << interface1.getPacketsReceived() 
                          << " packets (" << interface1.getBytesReceived() << " bytes), "
                          << "TX=" << interface1.getPacketsSent() 
                          << " packets (" << interface1.getBytesSent() << " bytes)" << std::endl;
                          
                std::cout << interface2.getName() << ": " 
                          << "RX=" << interface2.getPacketsReceived() 
                          << " packets (" << interface2.getBytesReceived() << " bytes), "
                          << "TX=" << interface2.getPacketsSent() 
                          << " packets (" << interface2.getBytesSent() << " bytes)" << std::endl;
            }
        });
        
        // Main loop - just wait for shutdown signal
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup
        std::cout << "\nShutting down..." << std::endl;
        interface1.stopCapture();
        interface2.stopCapture();
        
        if (stats_thread.joinable()) {
            stats_thread.join();
        }
        
        // Print final statistics
        std::cout << "\n--- Final Statistics ---" << std::endl;
        std::cout << interface1.getName() << ": " 
                  << "RX=" << interface1.getPacketsReceived() 
                  << " packets (" << interface1.getBytesReceived() << " bytes), "
                  << "TX=" << interface1.getPacketsSent() 
                  << " packets (" << interface1.getBytesSent() << " bytes)" << std::endl;
                  
        std::cout << interface2.getName() << ": " 
                  << "RX=" << interface2.getPacketsReceived() 
                  << " packets (" << interface2.getBytesReceived() << " bytes), "
                  << "TX=" << interface2.getPacketsSent() 
                  << " packets (" << interface2.getBytesSent() << " bytes)" << std::endl;
        
        std::cout << "Shutdown complete." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
