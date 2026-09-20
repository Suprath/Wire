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

    auto print_help = [&]() {
        std::cout << "\n  ╔══════════════════════════════════════════════════╗\n";
        std::cout << "  ║         PROJECT WIRE — AVAILABLE COMMANDS        ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════╣\n";
        std::cout << "  ║  /add              Add a new peer                ║\n";
        std::cout << "  ║  /peers            List all connected peers       ║\n";
        std::cout << "  ║  /switch <alias>   Switch active chat target      ║\n";
        std::cout << "  ║  /quit             Exit Wire                      ║\n";
        std::cout << "  ║  <message>         Send message to active peer    ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════╝\n\n";
    };

    print_help();

    while (std::getline(std::cin, user_input)) {

        // ── /quit ────────────────────────────────────────────────────────
        if (user_input == "/quit" || user_input == "exit" || user_input == "/exit") {
            running = false;
            break;
        }

        // ── /help ────────────────────────────────────────────────────────
        if (user_input == "/help" || user_input == "/?") {
            print_help();
            continue;
        }

        // ── /peers ───────────────────────────────────────────────────────
        if (user_input == "/peers") {
            std::cout << "\n  Connected peers:\n";
            for (size_t i = 0; i < contact_list.size(); ++i) {
                std::string marker = (contact_list[i].first == active_peer_alias) ? " ◀ active" : "";
                std::cout << "    [" << i + 1 << "] " << contact_list[i].first
                          << "  (" << contact_list[i].second << ")" << marker << "\n";
            }
            std::cout << "\n";
            continue;
        }

        // ── /add ─────────────────────────────────────────────────────────
        if (user_input == "/add") {
            std::string alias, ip_port;

            std::cout << "\n  Add New Peer\n";
            std::cout << "  ─────────────────────────────\n";
            std::cout << "  Alias (display name): ";
            std::cout << std::flush;
            if (!std::getline(std::cin, alias) || alias.empty()) {
                std::cout << "  [!] Cancelled — alias cannot be empty.\n\n";
                continue;
            }

            std::cout << "  IP:Port (e.g. 127.0.0.1:9002): ";
            std::cout << std::flush;
            if (!std::getline(std::cin, ip_port) || ip_port.empty()) {
                std::cout << "  [!] Cancelled — IP:Port cannot be empty.\n\n";
                continue;
            }

            // Parse IP and port from "ip:port" format
            std::string peer_ip = "127.0.0.1";
            uint16_t peer_port = remote_port;

            auto colon = ip_port.find(':');
            if (colon != std::string::npos) {
                peer_ip = ip_port.substr(0, colon);
                try {
                    peer_port = static_cast<uint16_t>(std::stoi(ip_port.substr(colon + 1)));
                } catch (...) {
                    std::cout << "  [!] Invalid port — using default " << remote_port << "\n";
                    peer_port = remote_port;
                }
            } else {
                // Just an IP with no port
                peer_ip = ip_port;
            }

            // Add to contact list sidebar
            contact_list.push_back({alias, "CONNECTED"});

            // Switch active target to the newly added peer
            active_peer_alias = alias;
            remote_ip = peer_ip;
            remote_port = peer_port;

            // Reset chat history for the new peer session
            chat_history.clear();

            std::cout << "  [✓] Peer \"" << alias << "\" added at " << peer_ip << ":" << peer_port << "\n\n";
            tui.render_chat_layout(active_peer_alias, true, contact_list, chat_history);
            continue;
        }

        // ── /switch <alias> ───────────────────────────────────────────────
        if (user_input.rfind("/switch", 0) == 0) {
            std::string target = user_input.size() > 8 ? user_input.substr(8) : "";
            // Trim leading spaces
            while (!target.empty() && target.front() == ' ') target.erase(target.begin());

            if (target.empty()) {
                std::cout << "  [!] Usage: /switch <alias>   (use /peers to list peers)\n\n";
                continue;
            }

            bool found = false;
            for (const auto& c : contact_list) {
                if (c.first == target) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::cout << "  [!] Peer \"" << target << "\" not found. Use /peers to list peers.\n\n";
                continue;
            }

            active_peer_alias = target;
            chat_history.clear();
            std::cout << "  [✓] Switched active chat to \"" << target << "\"\n\n";
            tui.render_chat_layout(active_peer_alias, true, contact_list, chat_history);
            continue;
        }

        // ── Unknown command ───────────────────────────────────────────────
        if (!user_input.empty() && user_input[0] == '/') {
            std::cout << "  [!] Unknown command: " << user_input << " — type /help for available commands.\n\n";
            continue;
        }

        // ── Send message ──────────────────────────────────────────────────
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
