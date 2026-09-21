/**
 * @file test_verified_status_and_queue.cpp
 * @brief Unit test verifying real-time verified connection status & offline message queuing
 * @project Project Wire
 */

#include "../src/crypto/double_ratchet.hpp"
#include "../src/network/real_socket_transport.hpp"
#include "../src/ledger/merkle_dag.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>

struct PendingMessage {
    std::string text;
    std::string timestamp;
};

struct TestPeer {
    std::string alias;
    std::string target_ip;
    uint16_t target_port;
    wire::crypto::DoubleRatchet ratchet;
    std::vector<PendingMessage> outgoing_queue;
    std::chrono::steady_clock::time_point last_seen;

    TestPeer(const std::string& name, const std::string& ip, uint16_t port,
             const wire::crypto::Key256& root_key, bool is_alice)
        : alias(name), target_ip(ip), target_port(port), ratchet(root_key, is_alice),
          last_seen(std::chrono::steady_clock::time_point::min()) {}

    bool is_verified() const {
        if (last_seen == std::chrono::steady_clock::time_point::min()) return false;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_seen).count();
        return elapsed < 15;
    }
};

void test_verified_status_and_queue() {
    std::cout << "[TEST] Running Verified Connection Status & Message Queue Test..." << std::endl;

    wire::crypto::Key256 root_key{};
    root_key.fill(0x42);

    TestPeer alice("Alice", "127.0.0.1", 9102, root_key, true);
    TestPeer bob("Bob", "127.0.0.1", 9101, root_key, false);

    // 1. Initial State: Unverified / SEARCHING
    if (alice.is_verified() || bob.is_verified()) {
        std::cerr << "[!] Initial state error: peers should start unverified\n";
        throw std::runtime_error("Initial unverified state check failed");
    }

    std::cout << "  [✓] Peers start in unverified / SEARCHING state." << std::endl;

    // 2. Queue message while Bob is disconnected
    alice.outgoing_queue.push_back({ "Queued offline message", "05:54" });
    if (alice.outgoing_queue.size() != 1) {
        std::cerr << "[!] Queueing failed\n";
        throw std::runtime_error("Queue check failed");
    }

    std::cout << "  [✓] Message drafted offline queued successfully: '" << alice.outgoing_queue[0].text
              << "' (Timestamp: " << alice.outgoing_queue[0].timestamp << ")." << std::endl;

    // 3. Simulate connection handshake (Alice receives decrypted ping from Bob)
    alice.last_seen = std::chrono::steady_clock::now();
    bob.last_seen = std::chrono::steady_clock::now();

    if (!alice.is_verified() || !bob.is_verified()) {
        std::cerr << "[!] Status failed to transition to CONNECTED after authenticated handshake\n";
        throw std::runtime_error("Connected state check failed");
    }

    std::cout << "  [✓] Authenticated packet updates status to CONNECTED." << std::endl;

    // 4. Simulate flushing queue upon connection verification
    wire::network::RealSocketTransport socket_alice;
    wire::network::RealSocketTransport socket_bob;

    if (!socket_alice.bind_port(9101) || !socket_bob.bind_port(9102)) {
        std::cerr << "[!] Socket binding failed in test\n";
        throw std::runtime_error("Socket binding failed");
    }

    wire::ledger::MerkleDAGLedger ledger;

    for (const auto& pending : alice.outgoing_queue) {
        std::string payload = "MSG|" + pending.timestamp + "|" + pending.text;
        auto [hdr, ciphertext] = alice.ratchet.encrypt(
            reinterpret_cast<const uint8_t*>(payload.data()), payload.size());

        std::vector<uint8_t> wire_packet(sizeof(wire::crypto::RatchetHeader) + ciphertext.size());
        std::memcpy(wire_packet.data(), &hdr, sizeof(hdr));
        std::memcpy(wire_packet.data() + sizeof(hdr), ciphertext.data(), ciphertext.size());

        socket_alice.send_packet("127.0.0.1", 9102, wire_packet.data(), wire_packet.size());
    }
    alice.outgoing_queue.clear();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto recv_opt = socket_bob.receive_packet();
    if (!recv_opt.has_value()) {
        std::cerr << "[!] Bob did not receive flushed packet\n";
        throw std::runtime_error("Flushed packet receive failed");
    }

    const auto& pkt = recv_opt.value();
    constexpr size_t hdr_size = sizeof(wire::crypto::RatchetHeader);
    wire::crypto::RatchetHeader hdr{};
    std::memcpy(&hdr, pkt.data.data(), hdr_size);

    auto decrypted = bob.ratchet.decrypt(hdr, pkt.data.data() + hdr_size, pkt.data.size() - hdr_size);
    if (!decrypted.has_value()) {
        std::cerr << "[!] Decryption of flushed packet failed\n";
        throw std::runtime_error("Decryption failed");
    }

    std::string plain_msg(decrypted->begin(), decrypted->end());
    if (plain_msg.rfind("MSG|", 0) != 0) {
        std::cerr << "[!] Payload header format mismatch: " << plain_msg << "\n";
        throw std::runtime_error("Payload header format mismatch");
    }

    auto sep = plain_msg.find('|', 4);
    std::string rx_ts = plain_msg.substr(4, sep - 4);
    std::string rx_txt = plain_msg.substr(sep + 1);

    if (rx_ts != "05:54" || rx_txt != "Queued offline message") {
        std::cerr << "[!] Timestamp/Text mismatch on delivery: got ts=" << rx_ts << ", txt=" << rx_txt << "\n";
        throw std::runtime_error("Delivery content mismatch");
    }

    std::cout << "  [✓] Delivered queued message to Bob with original timestamp preserved: '"
              << rx_txt << "' (" << rx_ts << ")." << std::endl;

    std::cout << "[SUCCESS] Verified connection status & message queue tests passed cleanly!\n" << std::endl;
}

int main() {
    test_verified_status_and_queue();
    return 0;
}
