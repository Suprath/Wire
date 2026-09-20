/**
 * @file real_socket_transport.cpp
 * @brief Implementation of Real Inter-Container UDP Socket Transport Engine
 * @project Project Wire
 */

#include "real_socket_transport.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
// MSVC does not define ssize_t — use a signed 64-bit alias
using wire_ssize_t = SSIZE_T;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
using wire_ssize_t = ssize_t;
#endif

namespace wire::network {

RealSocketTransport::RealSocketTransport() noexcept {
#if defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

RealSocketTransport::~RealSocketTransport() noexcept {
    if (m_sockfd >= 0) {
#if defined(_WIN32)
        closesocket(m_sockfd);
        WSACleanup();
#else
        close(m_sockfd);
#endif
    }
}

bool RealSocketTransport::bind_port(uint16_t port) noexcept {
    m_sockfd = static_cast<int>(socket(AF_INET, SOCK_DGRAM, 0));
    if (m_sockfd < 0) {
        return false;
    }

    // Set non-blocking mode
#if defined(_WIN32)
    u_long mode = 1;
    ioctlsocket(m_sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

    // Reuse address and enable broadcast
    int optval = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));
    setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&optval), sizeof(optval));
#if defined(SO_REUSEPORT)
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&optval), sizeof(optval));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return false;
    }

    m_local_port = port;
    return true;
}

bool RealSocketTransport::send_packet(const std::string& target_ip, uint16_t target_port, const uint8_t* data, size_t len) noexcept {
    if (m_sockfd < 0) {
        // Auto-bind if not bound yet
        if (!bind_port(0)) return false;
    }

    std::vector<std::string> candidates;
    if (!target_ip.empty()) {
        candidates.push_back(target_ip);
    }
    if (target_ip != "127.0.0.1") {
        candidates.push_back("127.0.0.1");
        candidates.push_back("255.255.255.255");
    }

    bool any_sent = false;

    for (const auto& candidate : candidates) {
        if (candidate.empty()) continue;

        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(target_port);

        bool resolved = false;

        if (inet_pton(AF_INET, candidate.c_str(), &dest.sin_addr) == 1) {
            resolved = true;
        } else {
            hostent* host = gethostbyname(candidate.c_str());
            if (host != nullptr && host->h_addr_list != nullptr && host->h_addr_list[0] != nullptr) {
                std::memcpy(&dest.sin_addr, host->h_addr_list[0], static_cast<size_t>(host->h_length));
                resolved = true;
            }
        }

        if (resolved && dest.sin_addr.s_addr != 0) {
            wire_ssize_t sent = sendto(m_sockfd, reinterpret_cast<const char*>(data), static_cast<int>(len), 0,
                                  reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
            if (sent == static_cast<wire_ssize_t>(len)) {
                any_sent = true;
            }
        }
    }

    return any_sent;
}

std::optional<SocketPacket> RealSocketTransport::receive_packet() noexcept {
    if (m_sockfd < 0) return std::nullopt;

    uint8_t buffer[2048];
    sockaddr_in sender_addr{};
    socklen_t addr_len = sizeof(sender_addr);

    wire_ssize_t recvd = recvfrom(m_sockfd, reinterpret_cast<char*>(buffer), static_cast<int>(sizeof(buffer)), 0,
                             reinterpret_cast<sockaddr*>(&sender_addr), &addr_len);

    if (recvd <= 0) {
        return std::nullopt; // No packet available in non-blocking mode
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sender_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

    SocketPacket pkt{};
    pkt.sender_ip = std::string(ip_str);
    pkt.sender_port = ntohs(sender_addr.sin_port);
    pkt.data.assign(buffer, buffer + recvd);

    return pkt;
}

} // namespace wire::network
