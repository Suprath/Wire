/**
 * @file test_real_socket_transport.cpp
 * @brief Unit tests for RealSocketTransport UDP network engine
 * @project Project Wire
 */

#include "../src/network/real_socket_transport.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <thread>
#include <chrono>

void test_real_socket_transport() {
    std::cout << "[TEST] Running RealSocketTransport UDP Socket Engine Test..." << std::endl;

    wire::network::RealSocketTransport peer1;
    wire::network::RealSocketTransport peer2;

    bool bound1 = peer1.bind_port(9005);
    bool bound2 = peer2.bind_port(9006);

    assert(bound1 && "Peer 1 failed to bind port 9005");
    assert(bound2 && "Peer 2 failed to bind port 9006");

    std::string test_payload = "PING_UDP_REAL_SOCKET_P2P";

    // Peer 1 sends packet to Peer 2 (port 9006)
    bool sent = peer1.send_packet("127.0.0.1", 9006, reinterpret_cast<const uint8_t*>(test_payload.data()), test_payload.size());
    if (!sent) {
        std::cerr << "Peer 1 failed to send packet\n";
        throw std::runtime_error("Peer 1 failed to send packet");
    }

    std::optional<wire::network::SocketPacket> packet_opt;
    for (int i = 0; i < 20; ++i) {
        packet_opt = peer2.receive_packet();
        if (packet_opt.has_value()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (!packet_opt.has_value()) {
        std::cerr << "Peer 2 did not receive packet in time\n";
        throw std::runtime_error("Peer 2 did not receive packet in time");
    }

    const auto& pkt = packet_opt.value();
    std::string received_str(pkt.data.begin(), pkt.data.end());
    if (received_str != test_payload) {
        std::cerr << "Payload mismatch: expected " << test_payload << ", got " << received_str << "\n";
        throw std::runtime_error("Payload mismatch");
    }

    std::cout << "  [PASS] Peer 1 -> Peer 2 UDP packet received cleanly: '" << received_str << "' (" << pkt.data.size() << " bytes)." << std::endl;
    std::cout << "[SUCCESS] RealSocketTransport tests passed cleanly!\n" << std::endl;
}

int main() {
    test_real_socket_transport();
    return 0;
}
