/**
 * @file terminal_ui.cpp
 * @brief Implementation of Terminal User Interface (TUI) Engine
 * @project Project Wire
 */

#include "terminal_ui.hpp"
#include <iostream>
#include <iomanip>

namespace wire::ui {

void TerminalUI::render_status_header(const std::string& peer_name, network::PeerState state, network::SearchTier tier) const noexcept {
    std::cout << "=======================================================================\n";
    std::cout << "  PROJECT WIRE — AETHER-seL4 SECURE P2P COMMUNICATIONS\n";
    std::cout << "  Target Peer: " << peer_name << "\n";
    std::cout << "  Status:      ";

    switch (state) {
        case network::PeerState::IDLE:      std::cout << "[ IDLE (Standby) ]\n"; break;
        case network::PeerState::SEARCHING: std::cout << "[ SEARCHING (Hunter Mode Active) ]\n"; break;
        case network::PeerState::CONNECTED: std::cout << "[ CONNECTED (Ghost Bridge Online) ]\n"; break;
        case network::PeerState::OFFLINE:   std::cout << "[ OFFLINE (Search Timeout Expired) ]\n"; break;
    }

    if (state == network::PeerState::SEARCHING) {
        std::cout << "  Active Tier: ";
        switch (tier) {
            case network::SearchTier::TIER_1_GENESIS_SUBNETS: std::cout << "Tier 1 (Genesis Subnets)\n"; break;
            case network::SearchTier::TIER_2_GLOBAL_BGP_SCAN: std::cout << "Tier 2 (Global BGP Prefix Scan @ 666 PPS)\n"; break;
            case network::SearchTier::TIER_3_NAT_HOLE_PUNCH:   std::cout << "Tier 3 (Bi-Directional NAT Hole Punching)\n"; break;
        }
    }
    std::cout << "=======================================================================\n" << std::endl;
}

void TerminalUI::render_chat_message(const std::string& sender, const std::string& text) const noexcept {
    std::cout << "[" << sender << "]: " << text << std::endl;
}

} // namespace wire::ui
