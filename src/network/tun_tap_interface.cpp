#include "sdr-network/network/tun_tap_interface.h"

#include <iostream>
#include <cstring>
#include <chrono>
#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#endif

namespace sdr_network {
namespace network {

TunTapInterface::TunTapInterface(const std::string& name, Type type)
    : name_(name), type_(type), fd_(-1) {
}

TunTapInterface::~TunTapInterface() {
    // Stop capture if running
    stopCapture();
    
    // Close the device if open
    if (fd_ >= 0) {
#ifdef __linux__
        close(fd_);
#endif
        fd_ = -1;
    }
}

TunTapInterface::TunTapInterface(TunTapInterface&& other) noexcept
    : name_(std::move(other.name_)),
      type_(other.type_),
      fd_(other.fd_),
      running_(other.running_.load()),
      capture_thread_(std::move(other.capture_thread_)),
      packets_received_(other.packets_received_.load()),
      packets_sent_(other.packets_sent_.load()),
      bytes_received_(other.bytes_received_.load()),
      bytes_sent_(other.bytes_sent_.load()) {
    // Reset the moved-from object
    other.fd_ = -1;
    other.running_ = false;
}

TunTapInterface& TunTapInterface::operator=(TunTapInterface&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        stopCapture();
        if (fd_ >= 0) {
#ifdef __linux__
            close(fd_);
#endif
        }
        
        // Move resources from other
        name_ = std::move(other.name_);
        type_ = other.type_;
        fd_ = other.fd_;
        running_ = other.running_.load();
        capture_thread_ = std::move(other.capture_thread_);
        packets_received_ = other.packets_received_.load();
        packets_sent_ = other.packets_sent_.load();
        bytes_received_ = other.bytes_received_.load();
        bytes_sent_ = other.bytes_sent_.load();
        
        // Reset the moved-from object
        other.fd_ = -1;
        other.running_ = false;
    }
    return *this;
}

bool TunTapInterface::initialize() {
#ifdef __linux__
    // Open the clone device
    fd_ = open("/dev/net/tun", O_RDWR);
    if (fd_ < 0) {
        std::cerr << "Failed to open /dev/net/tun: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Set up the device
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    
    // Set the interface name if specified
    if (!name_.empty()) {
        strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ - 1);
    }
    
    // Set flags based on device type
    ifr.ifr_flags = (type_ == Type::TUN) ? IFF_TUN : IFF_TAP;
    ifr.ifr_flags |= IFF_NO_PI; // No packet information
    
    // Create the device
    if (ioctl(fd_, TUNSETIFF, &ifr) < 0) {
        std::cerr << "Failed to create TUN/TAP device: " << strerror(errno) << std::endl;
        close(fd_);
        fd_ = -1;
        return false;
    }
    
    // Get the assigned interface name
    name_ = ifr.ifr_name;
    
    // Set non-blocking mode
    int flags = fcntl(fd_, F_GETFL);
    if (flags < 0 || fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Failed to set non-blocking mode: " << strerror(errno) << std::endl;
        close(fd_);
        fd_ = -1;
        return false;
    }
    
    std::cout << "Created " << (type_ == Type::TUN ? "TUN" : "TAP") 
              << " interface: " << name_ << std::endl;
    return true;
#else
    std::cerr << "TUN/TAP interfaces are only supported on Linux" << std::endl;
    return false;
#endif
}

bool TunTapInterface::configure(const std::string& ip_address, const std::string& netmask) {
#ifdef __linux__
    // Create a socket for interface configuration
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket for interface configuration: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Set up the interface flags (bring interface up)
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ - 1);
    
    // Get current flags
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        std::cerr << "Failed to get interface flags: " << strerror(errno) << std::endl;
        close(sock);
        return false;
    }
    
    // Set interface up
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        std::cerr << "Failed to set interface up: " << strerror(errno) << std::endl;
        close(sock);
        return false;
    }
    
    // Set IP address
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ - 1);
    
    struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    inet_pton(AF_INET, ip_address.c_str(), &addr->sin_addr);
    
    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0) {
        std::cerr << "Failed to set interface IP address: " << strerror(errno) << std::endl;
        close(sock);
        return false;
    }
    
    // Set netmask
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ - 1);
    
    addr = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_netmask);
    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    inet_pton(AF_INET, netmask.c_str(), &addr->sin_addr);
    
    if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0) {
        std::cerr << "Failed to set interface netmask: " << strerror(errno) << std::endl;
        close(sock);
        return false;
    }
    
    close(sock);
    std::cout << "Configured interface " << name_ 
              << " with IP " << ip_address 
              << " and netmask " << netmask << std::endl;
    return true;
#else
    std::cerr << "TUN/TAP interface configuration is only supported on Linux" << std::endl;
    return false;
#endif
}

bool TunTapInterface::startCapture(std::function<void(const std::vector<uint8_t>&)> packet_handler) {
    if (fd_ < 0) {
        std::cerr << "Cannot start capture: interface not initialized" << std::endl;
        return false;
    }
    
    if (running_) {
        std::cerr << "Capture already running" << std::endl;
        return false;
    }
    
    running_ = true;
    capture_thread_ = std::make_unique<std::thread>(&TunTapInterface::captureThreadFunc, this, packet_handler);
    std::cout << "Started packet capture on interface " << name_ << std::endl;
    return true;
}

void TunTapInterface::stopCapture() {
    if (running_ && capture_thread_) {
        running_ = false;
        if (capture_thread_->joinable()) {
            capture_thread_->join();
        }
        capture_thread_.reset();
        std::cout << "Stopped packet capture on interface " << name_ << std::endl;
    }
}

bool TunTapInterface::writePacket(const std::vector<uint8_t>& packet) {
    if (fd_ < 0 || !running_) {
        return false;
    }
    
#ifdef __linux__
    ssize_t written = write(fd_, packet.data(), packet.size());
    if (written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to write packet: " << strerror(errno) << std::endl;
        }
        return false;
    }
    
    packets_sent_++;
    bytes_sent_ += written;
    return true;
#else
    std::cerr << "writePacket is only supported on Linux" << std::endl;
    return false;
#endif
}

void TunTapInterface::captureThreadFunc(std::function<void(const std::vector<uint8_t>&)> packet_handler) {
#ifdef __linux__
    const size_t buffer_size = 2048; // Maximum packet size
    std::vector<uint8_t> buffer(buffer_size);
    
    while (running_) {
        ssize_t nread = read(fd_, buffer.data(), buffer.size());
        if (nread < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Error reading from interface: " << strerror(errno) << std::endl;
                // Small delay to avoid tight loop on errors
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            continue;
        }
        
        if (nread > 0) {
            packets_received_++;
            bytes_received_ += nread;
            
            // Create a packet of the correct size
            std::vector<uint8_t> packet(buffer.begin(), buffer.begin() + nread);
            
            // Call the packet handler
            try {
                packet_handler(packet);
            } catch (const std::exception& e) {
                std::cerr << "Exception in packet handler: " << e.what() << std::endl;
            }
        } else {
            // No data, small delay to avoid tight loop
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
#else
    // On non-Linux platforms, just exit the thread
    std::cerr << "Packet capture is only supported on Linux" << std::endl;
#endif
}

} // namespace network
} // namespace sdr_network
