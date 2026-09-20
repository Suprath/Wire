/**
 * @file interactive_tui.cpp
 * @brief Real P2P Socket Inter-Container Terminal UI Application for Project Wire
 * @project Project Wire
 * 
 * @details
 * Connects Peer A and Peer B via real UDP network sockets across Docker containers or host processes.
 * - Outgoing typed messages are encrypted with Double Ratchet and transmitted over real sockets.
 * - Incoming real UDP packets are received, authenticated, decrypted, and displayed live in the chat stream.
 */

#include "imessage_tui.hpp"
#include "../genesis/genesis_manager.hpp"
#include "../contact/contact_manager.hpp"
#include "../crypto/double_ratchet.hpp"
#include "../ledger/merkle_dag.hpp"
#include "../network/real_socket_transport.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>

int main() {
    wire::ui::iMessageTUI tui;
    wire::contact::ContactManager contacts;
    wire::network::RealSocketTransport socket;

    std::string my_peer_name = "Peer_A";
    const char* peer_env = std::getenv("PEER_NAME");
    if (peer_env != nullptr) {
        my_peer_name = peer_env;
    } else {
        std::cout << "=======================================================================\n";
        std::cout << "  PROJECT WIRE — AETHER-seL4 SECURE P2P INTERACTIVE CLIENT\n";
        std::cout << "=======================================================================\n\n";
        std::cout << "  Select Peer Role for this terminal session:\n";
        std::cout << "    [1] Peer A (Alice - Local UDP Port 9001 -> Remote 9002)\n";
        std::cout << "    [2] Peer B (Bob   - Local UDP Port 9002 -> Remote 9001)\n\n";
        std::cout << "  Select Choice [1-2] (default: 1): ";
        std::string choice;
        if (std::getline(std::cin, choice)) {
            if (choice == "2" || choice == "B" || choice == "b" || choice == "Peer_B" || choice == "bob" || choice == "Bob") {
                my_peer_name = "Peer_B";
            }
        }
    }

    uint16_t local_port = 9001;
    uint16_t remote_port = 9002;
    std::string remote_ip = "wire-peer-b";

    if (my_peer_name == "Peer_B") {
        local_port = 9002;
        remote_port = 9001;
        remote_ip = "wire-peer-a";
    }

    // Bind real local UDP socket
    if (!socket.bind_port(local_port)) {
        // Fallback for running both on localhost
        if (my_peer_name == "Peer_B") {
            local_port = 9004;
            remote_port = 9003;
        } else {
            local_port = 9003;
            remote_port = 9004;
        }
        socket.bind_port(local_port);
        remote_ip = "127.0.0.1";
    }

    // Shared root key for entanglement
    wire::crypto::Key256 shared_root_key{};
    shared_root_key.fill(0x55);

    bool is_alice = (my_peer_name == "Peer_A");
    wire::crypto::DoubleRatchet ratchet(shared_root_key, is_alice);
    wire::ledger::MerkleDAGLedger ledger;

    std::string active_peer_alias = is_alice ? "Peer B" : "Peer A";

    std::vector<std::pair<std::string, std::string>> contact_list = {
        {active_peer_alias, "CONNECTED"}
    };

    std::vector<wire::ui::ChatBubble> chat_history;

    tui.render_chat_layout(active_peer_alias, true, contact_list, chat_history);

    std::atomic<bool> running{true};

    // Background socket listener thread for receiving real incoming P2P messages
    std::thread listener_thread([&]() {
        while (running) {
            auto packet_opt = socket.receive_packet();
            if (packet_opt.has_value()) {
                const auto& pkt = packet_opt.value();

                constexpr size_t hdr_size = sizeof(wire::crypto::RatchetHeader);
                if (pkt.data.size() > hdr_size + 16) {
                    wire::crypto::RatchetHeader hdr{};
                    std::memcpy(&hdr, pkt.data.data(), hdr_size);

                    const uint8_t* ciphertext_ptr = pkt.data.data() + hdr_size;
                    size_t ciphertext_len = pkt.data.size() - hdr_size;

                    // Decrypt incoming ciphertext payload using received RatchetHeader
                    auto decrypted = ratchet.decrypt(hdr, ciphertext_ptr, ciphertext_len);

                    if (decrypted.has_value()) {
                        std::string plain_msg(decrypted->begin(), decrypted->end());
                        ledger.append_message(pkt.data.data(), pkt.data.size(), 1700000000);

                        // Add incoming green speech bubble
                        chat_history.push_back({ active_peer_alias, plain_msg, "19:20", false });
                        tui.render_chat_layout(active_peer_alias, true, contact_list, chat_history);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Keyboard input loop for outgoing messages
    std::string user_input;
    while (std::getline(std::cin, user_input)) {
        if (user_input == "/quit" || user_input == "exit" || user_input == "/exit") {
            running = false;
            break;
        }

        if (user_input.empty()) {
            tui.render_chat_layout(active_peer_alias, true, contact_list, chat_history);
            continue;
        }

        // 1. Encrypt message with Double Ratchet
        auto [hdr, ciphertext] = ratchet.encrypt(
            reinterpret_cast<const uint8_t*>(user_input.data()), user_input.size());

        // Construct wire packet: RatchetHeader (40 bytes) + Ciphertext
        std::vector<uint8_t> wire_packet(sizeof(wire::crypto::RatchetHeader) + ciphertext.size());
        std::memcpy(wire_packet.data(), &hdr, sizeof(hdr));
        std::memcpy(wire_packet.data() + sizeof(hdr), ciphertext.data(), ciphertext.size());

        // 2. Append to local Merkle-DAG ledger
        ledger.append_message(wire_packet.data(), wire_packet.size(), 1700000000);

        // 3. Transmit real encrypted packet over UDP network socket
        socket.send_packet(remote_ip, remote_port, wire_packet.data(), wire_packet.size());
        // Backup transmit to localhost if running on same machine
        socket.send_packet("127.0.0.1", remote_port, wire_packet.data(), wire_packet.size());

        // 4. Render outgoing cyan speech bubble
        chat_history.push_back({ "YOU", user_input, "19:20", true });
        tui.render_chat_layout(active_peer_alias, true, contact_list, chat_history);
    }

    running = false;
    if (listener_thread.joinable()) {
        listener_thread.join();
    }

    return 0;
}
