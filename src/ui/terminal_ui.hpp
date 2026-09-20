/**
 * @file terminal_ui.hpp
 * @brief Terminal User Interface (TUI) Application Engine for Project Wire
 * @project Project Wire
 * 
 * @details
 * Renders the Terminal UI client, displaying peer connection status, message logs,
 * search state indicators, and Genesis key provisioning commands.
 */

#ifndef WIRE_TERMINAL_UI_HPP
#define WIRE_TERMINAL_UI_HPP

#include <string>
#include <vector>
#include <iostream>
#include "../network/discovery_engine.hpp"

namespace wire::ui {

class TerminalUI {
public:
    TerminalUI() = default;

    /**
     * @brief Displays the Project Wire header banner and status indicator dashboard.
     * @param peer_name Target peer handle.
     * @param state Current peer state.
     * @param tier Current search tier.
     */
    void render_status_header(const std::string& peer_name, network::PeerState state, network::SearchTier tier) const noexcept;

    /**
     * @brief Appends a chat message to the terminal display log.
     * @param sender Message sender label (e.g. "YOU", "PEER_B").
     * @param text Message body.
     */
    void render_chat_message(const std::string& sender, const std::string& text) const noexcept;
};

} // namespace wire::ui

#endif // WIRE_TERMINAL_UI_HPP
