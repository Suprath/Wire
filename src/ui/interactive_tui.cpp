/**
 * @file interactive_tui.cpp
 * @brief Real P2P Socket Inter-Container Terminal UI Application for Project Wire
 * @project Project Wire
 */

#include "imessage_tui.hpp"
#include "../genesis/genesis_manager.hpp"
#include "../contact/contact_manager.hpp"
#include "../crypto/double_ratchet.hpp"
#include "../ledger/merkle_dag.hpp"
#include "../network/real_socket_transport.hpp"
#include "../update/update_manager.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <random>
#include <iomanip>
#include <sstream>
#include <memory>
#include <mutex>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

/**
 * @struct PeerSession
 * @brief Encapsulates an active peer contact session and crypto state.
 */
struct PeerSession {
    std::string alias;
    std::string target_ip;
    uint16_t target_port;
    wire::crypto::DoubleRatchet ratchet;
    std::vector<wire::ui::ChatBubble> chat_history;

    PeerSession(const std::string& name, const std::string& ip, uint16_t port,
                const wire::crypto::Key256& root_key, bool is_alice)
        : alias(name), target_ip(ip), target_port(port), ratchet(root_key, is_alice) {}
};

/** Derive a 256-bit key from a user passphrase */
wire::crypto::Key256 derive_key_from_passphrase(const std::string& passphrase) {
    wire::crypto::Key256 key{};
    key.fill(0xAA); // Initial salt
    for (size_t i = 0; i < passphrase.size(); ++i) {
        uint8_t c = static_cast<uint8_t>(passphrase[i]);
        key[i % 32] ^= c;
        key[(i * 7 + 3) % 32] = static_cast<uint8_t>(key[(i * 7 + 3) % 32] + c + i);
    }
    return key;
}

} // anonymous namespace

int main() {
#if defined(_WIN32)
    // Enable UTF-8 input and output — fixes box-drawing characters in Windows terminal
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    // ── Load or generate persistent identity key ──────────────────────────────
    std::string data_dir = wire::update::UpdateManager::get_user_data_directory();
    std::filesystem::path identity_path = std::filesystem::path(data_dir) / "identity.key";
    std::filesystem::create_directories(data_dir);

    wire::crypto::Key256 my_identity{};

    std::ifstream idf(identity_path, std::ios::binary);
    if (idf.is_open() && idf.read(reinterpret_cast<char*>(my_identity.data()), 32).gcount() == 32) {
        idf.close();
    } else {
        std::random_device rd;
        std::mt19937_64 rng(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (auto& b : my_identity) b = dist(rng);

        std::ofstream odf(identity_path, std::ios::binary);
        if (odf.is_open()) {
            odf.write(reinterpret_cast<const char*>(my_identity.data()), 32);
        }
    }

    std::ostringstream hex_ss;
    hex_ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < my_identity.size(); ++i) {
        hex_ss << std::setw(2) << static_cast<unsigned>(my_identity[i]);
    }
    std::string my_identity_hex = hex_ss.str();

    wire::ui::iMessageTUI tui;
    wire::network::RealSocketTransport socket;
    wire::ledger::MerkleDAGLedger ledger;

    // ── 1. Startup Prompts: Nickname & Local Port ────────────────────────────
    std::cout << "=======================================================================\n";
    std::cout << "  PROJECT WIRE — AETHER-seL4 SECURE P2P INTERACTIVE CLIENT\n";
    std::cout << "=======================================================================\n\n";

    std::string my_nickname = "User";
    const char* nick_env = std::getenv("MY_NICKNAME");
    if (nick_env == nullptr) nick_env = std::getenv("PEER_NAME");

    if (nick_env != nullptr) {
        my_nickname = nick_env;
    } else {
        std::cout << "  Enter your local nickname (e.g. Alice): ";
        std::cout << std::flush;
        std::string input_nick;
        if (std::getline(std::cin, input_nick) && !input_nick.empty()) {
            my_nickname = input_nick;
        }
    }

    uint16_t local_port = 9001;
    const char* port_env = std::getenv("LOCAL_PORT");
    if (port_env != nullptr) {
        try { local_port = static_cast<uint16_t>(std::stoi(port_env)); } catch (...) {}
    } else if (nick_env == nullptr) {
        std::cout << "  Enter your local UDP listening port (default 9001): ";
        std::cout << std::flush;
        std::string input_port;
        if (std::getline(std::cin, input_port) && !input_port.empty()) {
            try { local_port = static_cast<uint16_t>(std::stoi(input_port)); } catch (...) {}
        }
    }

    // Bind real local UDP socket
    if (!socket.bind_port(local_port)) {
        std::cout << "  [!] Port " << local_port << " busy, trying " << local_port + 1 << "...\n";
        local_port += 1;
        if (!socket.bind_port(local_port)) {
            std::cout << "  [!] Error: Could not bind local UDP socket on port " << local_port << "\n";
            return 1;
        }
    }

    std::cout << "\n  [✓] Listening on UDP port " << local_port << " as \"" << my_nickname << "\".\n\n";

    // ── 2. Session Management ────────────────────────────────────────────────
    std::mutex session_mutex;
    std::vector<std::shared_ptr<PeerSession>> sessions;
    int active_session_index = -1;

    auto update_layout = [&]() {
        std::vector<std::pair<std::string, std::string>> contact_list;
        for (size_t i = 0; i < sessions.size(); ++i) {
            std::string status = (static_cast<int>(i) == active_session_index) ? "CONNECTED" : "IDLE";
            contact_list.push_back({sessions[i]->alias, status});
        }

        if (active_session_index >= 0 && active_session_index < static_cast<int>(sessions.size())) {
            auto s = sessions[static_cast<size_t>(active_session_index)];
            tui.render_chat_layout(s->alias, true, contact_list, s->chat_history);
        } else {
            std::vector<wire::ui::ChatBubble> empty_history;
            std::vector<std::pair<std::string, std::string>> empty_contacts = {
                {"No Connected Peers", "USE /add"}
            };
            tui.render_chat_layout("No Connected Peers (Use /add)", false, empty_contacts, empty_history);
        }
    };

    update_layout();

    std::atomic<bool> running{true};

    // ── 3. Background Socket Listener ───────────────────────────────────────
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

                    std::lock_guard<std::mutex> lock(session_mutex);
                    for (size_t i = 0; i < sessions.size(); ++i) {
                        auto& s = sessions[i];
                        auto decrypted = s->ratchet.decrypt(hdr, ciphertext_ptr, ciphertext_len);

                        if (decrypted.has_value()) {
                            std::string plain_msg(decrypted->begin(), decrypted->end());
                            ledger.append_message(pkt.data.data(), pkt.data.size(), 1700000000);

                            s->chat_history.push_back({ s->alias, plain_msg, "19:20", false });

                            if (static_cast<int>(i) == active_session_index) {
                                update_layout();
                            }
                            break;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // ── 4. Keyboard Command & Message Loop ───────────────────────────────────
    auto print_help = [&]() {
        std::cout << "\n  ╔══════════════════════════════════════════════════╗\n";
        std::cout << "  ║         PROJECT WIRE — AVAILABLE COMMANDS        ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════╣\n";
        std::cout << "  ║  /myid             Show your identity & port     ║\n";
        std::cout << "  ║  /add              Add a new peer with secret    ║\n";
        std::cout << "  ║  /peers            List all connected peers      ║\n";
        std::cout << "  ║  /switch <alias>   Switch active chat target     ║\n";
        std::cout << "  ║  /update           Check & install latest update ║\n";
        std::cout << "  ║  /quit             Exit Wire                     ║\n";
        std::cout << "  ║  <message>         Send message to active peer   ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════╝\n\n";
    };

    print_help();

    std::string user_input;
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

        // ── /myid ────────────────────────────────────────────────────────
        if (user_input == "/myid") {
            std::cout << "\n  ╔═════════════════════════════════════════════════════════════════════════════════╗\n";
            std::cout << "  ║                           YOUR WIRE IDENTITY                                    ║\n";
            std::cout << "  ╠═════════════════════════════════════════════════════════════════════════════════╣\n";
            std::cout << "  ║  Nickname: " << my_nickname << "\n";
            std::cout << "  ║  Hash:     " << my_identity_hex << "\n";
            std::cout << "  ║  Port:     " << local_port << "\n";
            std::cout << "  ╠═════════════════════════════════════════════════════════════════════════════════╣\n";
            std::cout << "  ║  Share your IP + Port + Shared Secret with your peer to /add.                   ║\n";
            std::cout << "  ╚═════════════════════════════════════════════════════════════════════════════════╝\n\n";
            continue;
        }

        // ── /peers ───────────────────────────────────────────────────────
        if (user_input == "/peers") {
            std::lock_guard<std::mutex> lock(session_mutex);
            if (sessions.empty()) {
                std::cout << "\n  No connected peers. Use /add to connect to a peer.\n\n";
            } else {
                std::cout << "\n  Connected peers:\n";
                for (size_t i = 0; i < sessions.size(); ++i) {
                    std::string marker = (static_cast<int>(i) == active_session_index) ? " ◀ active" : "";
                    std::cout << "    [" << i + 1 << "] " << sessions[i]->alias
                              << "  (" << sessions[i]->target_ip << ":" << sessions[i]->target_port << ")"
                              << marker << "\n";
                }
                std::cout << "\n";
            }
            continue;
        }

        // ── /update ──────────────────────────────────────────────────────
        if (user_input == "/update") {
            std::cout << "\n  Checking for updates...\n";
            static_cast<void>(wire::update::UpdateManager::download_and_apply_update());
            continue;
        }

        // ── /add ─────────────────────────────────────────────────────────
        if (user_input == "/add") {
            std::string alias, ip_port, passphrase, role_choice;

            std::cout << "\n  Add New Peer Setup\n";
            std::cout << "  ─────────────────────────────\n";
            std::cout << "  Peer Nickname (display name): ";
            std::cout << std::flush;
            if (!std::getline(std::cin, alias) || alias.empty()) {
                std::cout << "  [!] Cancelled — alias cannot be empty.\n\n";
                continue;
            }

            std::cout << "  Target IP:Port (e.g. 192.168.1.5:9001): ";
            std::cout << std::flush;
            if (!std::getline(std::cin, ip_port) || ip_port.empty()) {
                std::cout << "  [!] Cancelled — IP:Port cannot be empty.\n\n";
                continue;
            }

            std::cout << "  Shared Passphrase / Secret Key (must match peer's key): ";
            std::cout << std::flush;
            if (!std::getline(std::cin, passphrase) || passphrase.empty()) {
                std::cout << "  [!] Cancelled — secret key cannot be empty.\n\n";
                continue;
            }

            std::cout << "  Your Role for this connection:\n";
            std::cout << "    [1] Initiator (Alice)\n";
            std::cout << "    [2] Responder (Bob)\n";
            std::cout << "  Select Choice [1-2] (default: 1): ";
            std::cout << std::flush;
            bool is_alice = true;
            if (std::getline(std::cin, role_choice)) {
                if (role_choice == "2" || role_choice == "B" || role_choice == "b" || role_choice == "Bob" || role_choice == "bob") {
                    is_alice = false;
                }
            }

            // Parse IP and port
            std::string peer_ip = "127.0.0.1";
            uint16_t peer_port = 9001;

            auto colon = ip_port.find(':');
            if (colon != std::string::npos) {
                peer_ip = ip_port.substr(0, colon);
                try {
                    peer_port = static_cast<uint16_t>(std::stoi(ip_port.substr(colon + 1)));
                } catch (...) {
                    std::cout << "  [!] Invalid port — using default 9001\n";
                }
            } else {
                peer_ip = ip_port;
            }

            wire::crypto::Key256 derived_key = derive_key_from_passphrase(passphrase);

            {
                std::lock_guard<std::mutex> lock(session_mutex);
                auto new_session = std::make_shared<PeerSession>(alias, peer_ip, peer_port, derived_key, is_alice);
                sessions.push_back(new_session);
                active_session_index = static_cast<int>(sessions.size()) - 1;
            }

            std::cout << "  [✓] Peer \"" << alias << "\" added at " << peer_ip << ":" << peer_port << "\n\n";
            update_layout();
            continue;
        }

        // ── /switch <alias> ───────────────────────────────────────────────
        if (user_input.rfind("/switch", 0) == 0) {
            std::string target = user_input.size() > 8 ? user_input.substr(8) : "";
            while (!target.empty() && target.front() == ' ') target.erase(target.begin());

            if (target.empty()) {
                std::cout << "  [!] Usage: /switch <alias>   (use /peers to list peers)\n\n";
                continue;
            }

            std::lock_guard<std::mutex> lock(session_mutex);
            bool found = false;
            for (size_t i = 0; i < sessions.size(); ++i) {
                if (sessions[i]->alias == target) {
                    active_session_index = static_cast<int>(i);
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::cout << "  [!] Peer \"" << target << "\" not found. Use /peers to list peers.\n\n";
                continue;
            }

            std::cout << "  [✓] Switched active chat to \"" << target << "\"\n\n";
            update_layout();
            continue;
        }

        // ── Unknown command ───────────────────────────────────────────────
        if (!user_input.empty() && user_input[0] == '/') {
            std::cout << "  [!] Unknown command: " << user_input << " — type /help for available commands.\n\n";
            continue;
        }

        // ── Send message ──────────────────────────────────────────────────
        std::lock_guard<std::mutex> lock(session_mutex);
        if (active_session_index < 0 || active_session_index >= static_cast<int>(sessions.size())) {
            std::cout << "  [!] No peer connected. Use /add to connect to a peer first.\n\n";
            continue;
        }

        auto active_session = sessions[static_cast<size_t>(active_session_index)];

        if (user_input.empty()) {
            update_layout();
            continue;
        }

        // 1. Encrypt message with Double Ratchet
        auto [hdr, ciphertext] = active_session->ratchet.encrypt(
            reinterpret_cast<const uint8_t*>(user_input.data()), user_input.size());

        // Construct wire packet: RatchetHeader (40 bytes) + Ciphertext
        std::vector<uint8_t> wire_packet(sizeof(wire::crypto::RatchetHeader) + ciphertext.size());
        std::memcpy(wire_packet.data(), &hdr, sizeof(hdr));
        std::memcpy(wire_packet.data() + sizeof(hdr), ciphertext.data(), ciphertext.size());

        // 2. Append to local Merkle-DAG ledger
        ledger.append_message(wire_packet.data(), wire_packet.size(), 1700000000);

        // 3. Transmit real encrypted packet over targeted UDP network socket (NO BROADCAST)
        socket.send_packet(active_session->target_ip, active_session->target_port,
                           wire_packet.data(), wire_packet.size());

        // 4. Render outgoing speech bubble
        active_session->chat_history.push_back({ "YOU", user_input, "19:20", true });
        update_layout();
    }

    running = false;
    if (listener_thread.joinable()) {
        listener_thread.join();
    }

    return 0;
}
