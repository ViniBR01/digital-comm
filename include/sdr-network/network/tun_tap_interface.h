#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <cstdint>

namespace sdr_network {
namespace network {

/**
 * @brief Class for creating and managing TUN/TAP network interfaces in Linux
 * 
 * This class provides functionality to create virtual network interfaces
 * that can be used to send/receive IP packets from userspace applications.
 * - TUN devices operate at IP level (layer 3)
 * - TAP devices operate at Ethernet level (layer 2)
 */
class TunTapInterface {
public:
    /**
     * @brief Type of interface to create
     */
    enum class Type {
        TUN,  // IP level device
        TAP   // Ethernet level device
    };

    /**
     * @brief Constructor
     * 
     * @param name Name of the interface to create
     * @param type Type of interface (TUN or TAP)
     */
    TunTapInterface(const std::string& name, Type type);

    /**
     * @brief Destructor - closes the interface if open
     */
    ~TunTapInterface();

    // Non-copyable
    TunTapInterface(const TunTapInterface&) = delete;
    TunTapInterface& operator=(const TunTapInterface&) = delete;

    // Movable
    TunTapInterface(TunTapInterface&&) noexcept;
    TunTapInterface& operator=(TunTapInterface&&) noexcept;

    /**
     * @brief Initialize the interface
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Set IP address and netmask for the interface
     * 
     * @param ip_address IP address in dotted decimal notation
     * @param netmask Netmask in dotted decimal notation
     * @return true if successful, false otherwise
     */
    bool configure(const std::string& ip_address, const std::string& netmask);
    
    /**
     * @brief Start packet capture in a separate thread
     * 
     * @param packet_handler Callback function that will be called for each received packet
     * @return true if capture started successfully, false otherwise
     */
    bool startCapture(std::function<void(const std::vector<uint8_t>&)> packet_handler);
    
    /**
     * @brief Stop packet capture
     */
    void stopCapture();
    
    /**
     * @brief Write a packet to the interface
     * 
     * @param packet Data to write
     * @return true if successful, false otherwise
     */
    bool writePacket(const std::vector<uint8_t>& packet);
    
    /**
     * @brief Get interface name
     * 
     * @return Interface name
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief Get interface statistics
     */
    uint64_t getPacketsReceived() const { return packets_received_; }
    uint64_t getPacketsSent() const { return packets_sent_; }
    uint64_t getBytesReceived() const { return bytes_received_; }
    uint64_t getBytesSent() const { return bytes_sent_; }
    
    /**
     * @brief Check if interface is running
     */
    bool isRunning() const { return running_; }

private:
    std::string name_;
    Type type_;
    int fd_ = -1;
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> capture_thread_;
    
    // Statistics
    std::atomic<uint64_t> packets_received_{0};
    std::atomic<uint64_t> packets_sent_{0};
    std::atomic<uint64_t> bytes_received_{0};
    std::atomic<uint64_t> bytes_sent_{0};
    
    // Packet capture thread function
    void captureThreadFunc(std::function<void(const std::vector<uint8_t>&)> packet_handler);
};

} // namespace network
} // namespace sdr_network
