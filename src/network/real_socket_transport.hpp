/**
 * @file real_socket_transport.hpp
 * @brief Real Inter-Container UDP Socket Transport Engine for P2P Messaging
 * @project Project Wire
 * 
 * @details
 * Handles real network UDP socket creation, binding, listening, and transmission
 * between concurrent Docker container instances (Peer A and Peer B).
 */

#ifndef WIRE_REAL_SOCKET_TRANSPORT_HPP
#define WIRE_REAL_SOCKET_TRANSPORT_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <thread>
#include <atomic>

namespace wire::network {

/**
 * @struct SocketPacket
 * @brief Represents a raw network packet received over UDP socket.
 */
struct SocketPacket {
    std::string sender_ip;     /**< IP address of remote sender */
    uint16_t sender_port;      /**< UDP port of remote sender */
    std::vector<uint8_t> data; /**< Packet payload bytes */
};

/**
 * @class RealSocketTransport
 * @brief Manages non-blocking UDP socket binding, transmission, and reception for real P2P messaging.
 */
class RealSocketTransport {
public:
    RealSocketTransport() noexcept;
    ~RealSocketTransport() noexcept;

    /**
     * @brief Binds local UDP socket to specified port.
     * @param port Local UDP port to bind (e.g. 9001 for Peer A, 9002 for Peer B).
     * @return true if successfully bound, false if error.
     */
    bool bind_port(uint16_t port) noexcept;

    /**
     * @brief Transmits a raw packet to a target IP address and UDP port over the network.
     * @param target_ip Target IP address or hostname (e.g. "wire-peer-b" or "127.0.0.1").
     * @param target_port Target UDP port.
     * @param data Payload byte array.
     * @param len Payload length.
     * @return true if successfully sent, false if socket send error.
     */
    bool send_packet(const std::string& target_ip, uint16_t target_port, const uint8_t* data, size_t len) noexcept;

    /**
     * @brief Non-blocking receive for incoming UDP packets.
     * @return std::optional<SocketPacket> Packet if available, nullopt if no packet in buffer.
     */
    std::optional<SocketPacket> receive_packet() noexcept;

    /**
     * @brief Retrieves local bound UDP port.
     * @return uint16_t Port number.
     */
    [[nodiscard]] uint16_t local_port() const noexcept { return m_local_port; }

private:
    int m_sockfd{-1};
    uint16_t m_local_port{0};
};

} // namespace wire::network

#endif // WIRE_REAL_SOCKET_TRANSPORT_HPP
