/**
 * @file imessage_tui.cpp
 * @brief Implementation of Terminal UI/UX Screens (Genesis Setup & Chat)
 * @project Project Wire
 */

#include "imessage_tui.hpp"
#include <iostream>
#include <iomanip>

namespace wire::ui {

void iMessageTUI::render_main_menu(const std::string& my_pubkey_hex, size_t contact_count) const noexcept {
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "=======================================================================\n";
    std::cout << "  PROJECT WIRE — AETHER-seL4 SECURE P2P COMMUNICATIONS DASHBOARD\n";
    std::cout << "  Identity Key Hash: " << my_pubkey_hex.substr(0, 24) << "...\n";
    std::cout << "  Saved Contacts:    " << contact_count << " Peer(s)\n";
    std::cout << "=======================================================================\n\n";

    std::cout << "  [1] 🔑 View My Genesis Entanglement Key & QR Payload\n";
    std::cout << "  [2] ➕ Add New Peer Contact (Paste Genesis Key / Hash & Set Name)\n";
    std::cout << "  [3] 💬 Open Active Peer Chat Session\n";
    std::cout << "  [4] 🚪 Exit Session\n\n";
    std::cout << "  Select Option [1-4]: ";
}

void iMessageTUI::render_my_genesis_key(const std::string& armored_key_text, const std::string& qr_payload) const noexcept {
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "=======================================================================\n";
    std::cout << "  MY GENESIS ENTANGLEMENT DATA (Share with your Peer)\n";
    std::cout << "=======================================================================\n\n";

    std::cout << "--- BASE64 ARMORED KEY FILE (wire_genesis.key) ---\n";
    std::cout << armored_key_text << "\n";

    std::cout << "--- COMPACT QR CODE STRING PAYLOAD ---\n";
    std::cout << qr_payload << "\n\n";

    std::cout << "=======================================================================\n";
    std::cout << "  Press ENTER to return to Main Menu...";
}

void iMessageTUI::render_add_peer_screen() const noexcept {
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "=======================================================================\n";
    std::cout << "  ADD NEW PEER ENTANGLEMENT CONTACT\n";
    std::cout << "=======================================================================\n\n";
    std::cout << "  Instruction: Paste your peer's Armored Key File string or QR payload below.\n\n";
}

void iMessageTUI::render_chat_layout(
    const std::string& active_peer,
    bool is_connected,
    const std::vector<std::pair<std::string, std::string>>& contacts,
    const std::vector<ChatBubble>& history) const noexcept {

    std::cout << "\033[2J\033[H"; // Clear terminal screen

    // Top System Header Bar
    std::cout << "┌──────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│  WIRE  │  🔒 seL4 Micro-VM  │  Peer: "
              << std::left << std::setw(15) << active_peer
              << " │ Status: "
              << (is_connected ? "\033[32m● CONNECTED\033[0m" : "\033[33m○ SEARCHING\033[0m")
              << std::right << std::setw(18) << "│\n";
    std::cout << "├──────────────────────┬───────────────────────────────────────────────────────┤\n";

    // Split Layout: Left Contact Sidebar vs Right Chat Window
    size_t max_rows = std::max(contacts.size() + 2, history.size() * 3 + 2);
    if (max_rows < 10) max_rows = 10;

    for (size_t row = 0; row < max_rows; ++row) {
        // Render Left Sidebar Column
        std::cout << "│ ";
        if (row < contacts.size()) {
            std::string status_badge = (contacts[row].second == "CONNECTED") ? "\033[32m●\033[0m" : "○";
            std::string name = contacts[row].first;
            if (name.size() > 14) name = name.substr(0, 14);
            std::cout << status_badge << " " << std::left << std::setw(16) << name << " │ ";
        } else if (row == contacts.size() && contacts.size() > 0) {
            std::cout << "-------------------- │ ";
        } else {
            std::cout << std::left << std::setw(20) << "" << " │ ";
        }

        // Render Right Chat Bubbles Column
        size_t bubble_idx = row / 3;
        size_t bubble_line = row % 3;

        if (bubble_idx < history.size()) {
            const auto& bubble = history[bubble_idx];
            if (bubble.is_outgoing) {
                // Right-aligned Outgoing Speech Bubble (Cyan)
                if (bubble_line == 0) {
                    std::cout << "\033[36m                       ┌────────────────────────────────┐\033[0m";
                } else if (bubble_line == 1) {
                    std::string text = bubble.message;
                    if (text.size() > 28) text = text.substr(0, 25) + "...";
                    std::cout << "\033[36m                       │ \033[0m" << std::left << std::setw(30) << text << "\033[36m│\033[0m";
                } else {
                    std::cout << "\033[36m                       └────────────────────────────────┘\033[0m";
                }
            } else {
                // Left-aligned Incoming Speech Bubble (Green)
                if (bubble_line == 0) {
                    std::cout << "\033[32m  ┌────────────────────────────────┐\033[0m";
                } else if (bubble_line == 1) {
                    std::string text = bubble.message;
                    if (text.size() > 28) text = text.substr(0, 25) + "...";
                    std::cout << "\033[32m  │ \033[0m" << std::left << std::setw(30) << text << "\033[32m│\033[0m";
                } else {
                    std::cout << "\033[32m  └────────────────────────────────┘\033[0m";
                }
            }
        }
        std::cout << "\n";
    }

    std::cout << "├──────────────────────┴───────────────────────────────────────────────────────┤\n";
    std::cout << "│ 🔒 Message #" << active_peer << " (type /menu to return, /add to add peer): " << std::flush;
}

} // namespace wire::ui
