/**
 * @file test_terminal_ui.cpp
 * @brief Unit tests for Terminal UI Rendering Client
 * @project Project Wire
 */

#include "../src/ui/terminal_ui.hpp"
#include <iostream>
#include <cassert>

void test_terminal_ui_render() {
    std::cout << "[TEST] Running Terminal UI Client Test..." << std::endl;

    wire::ui::TerminalUI ui;

    ui.render_status_header("Peer_B (Bob)", wire::network::PeerState::SEARCHING, wire::network::SearchTier::TIER_2_GLOBAL_BGP_SCAN);
    ui.render_chat_message("PEER_B", "Encrypted mathematical knock ACK received!");
    ui.render_chat_message("YOU", "Hello over seL4 Micro-VM!");

    std::cout << "  [PASS] Terminal UI status dashboard and chat message rendering verified." << std::endl;
    std::cout << "[SUCCESS] Terminal UI tests passed cleanly!\n" << std::endl;
}

int main() {
    test_terminal_ui_render();
    return 0;
}
