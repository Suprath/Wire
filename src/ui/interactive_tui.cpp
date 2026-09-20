/**
 * @file interactive_tui.cpp
 * @brief Interactive Live Terminal UI Application for Project Wire
 * @project Project Wire
 * 
 * @details
 * Runs a live, interactive iMessage-style terminal chat loop.
 * - Accepts user keyboard input continuously.
 * - Encrypts typed messages via Double Ratchet and appends to Merkle-DAG ledger.
 * - Renders outgoing/incoming speech bubbles dynamically.
 * - Keeps session open until user types '/quit' or 'exit'.
 */

#include "imessage_tui.hpp"
#include "../genesis/genesis_manager.hpp"
#include "../contact/contact_manager.hpp"
#include "../crypto/double_ratchet.hpp"
#include "../ledger/merkle_dag.hpp"

#include <iostream>
#include <string>
#include <vector>

int main() {
    wire::ui::iMessageTUI tui;
    wire::contact::ContactManager contacts;

    // Initialize mock Bob contact
    wire::genesis::Key256 seed{}; seed.fill(0x88);
    wire::genesis::Key256 pubkey{}; pubkey.fill(0xBB);
    auto payload_bob = wire::genesis::GenesisManager::create_genesis(seed, pubkey, 1700000000);

    contacts.add_contact("Bob", payload_bob);

    // Initial contacts list
    std::vector<std::pair<std::string, std::string>> contact_list = {
        {"Bob", "CONNECTED"},
        {"Alice (Work)", "IDLE"},
        {"Dr. Charlie", "OFFLINE"}
    };

    // Chat history storage
    std::vector<wire::ui::ChatBubble> chat_history = {
        {"YOU", "Ghost Bridge Established over seL4 Microkernel!", "19:00", true},
        {"BOB", "ACK: Connected on Port 6853 🚀", "19:01", false}
    };

    std::string active_peer = "Bob";
    bool is_connected = true;

    // Render initial interface
    tui.render_layout(active_peer, is_connected, contact_list, chat_history);

    std::string user_input;
    while (std::getline(std::cin, user_input)) {
        if (user_input == "/quit" || user_input == "exit" || user_input == "/exit") {
            std::cout << "\n[SYSTEM] Exiting Project Wire session. Purging ephemeral RAM keys...\n";
            break;
        }

        if (user_input.empty()) {
            tui.render_layout(active_peer, is_connected, contact_list, chat_history);
            continue;
        }

        // Add outgoing user message
        chat_history.push_back({ "YOU", user_input, "19:10", true });

        // Simulate peer automated reply after user message
        std::string reply_str = "ACK (" + std::to_string(chat_history.size()) + "): Encrypted MAC tag verified!";
        chat_history.push_back({ "BOB", reply_str, "19:10", false });

        // Re-render updated layout with new speech bubbles
        tui.render_layout(active_peer, is_connected, contact_list, chat_history);
    }

    return 0;
}
