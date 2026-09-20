/**
 * @file imessage_tui.hpp
 * @brief Minimal, iMessage-Style Modern Terminal User Interface for Project Wire
 * @project Project Wire
 * 
 * @details
 * Renders a clean, minimal, multi-pane TUI layout inspired by iMessage / Signal:
 * - Left Sidebar: Contact List with online status badges.
 * - Right Main Pane: Active Peer Header (🔒 seL4 Verified), Right/Left aligned Speech Bubbles.
 * - Footer: Sleek single-line input prompt.
 */

#ifndef WIRE_IMESSAGE_TUI_HPP
#define WIRE_IMESSAGE_TUI_HPP

#include <string>
#include <vector>
#include <memory>

namespace wire::ui {

struct ChatBubble {
    std::string sender;    /**< "YOU" or peer display name */
    std::string message;   /**< Text content */
    std::string timestamp; /**< Formatted time (HH:MM) */
    bool is_outgoing;      /**< true if sent by local user (right-aligned cyan bubble) */
};

class iMessageTUI {
public:
    iMessageTUI() = default;

    /**
     * @brief Renders the complete minimal iMessage-style terminal interface layout.
     * @param active_peer Active peer display name (e.g. "Bob").
     * @param is_connected Connection status.
     * @param contacts List of contact tuples (Name, StatusBadge).
     * @param history Conversation message bubbles.
     */
    void render_layout(
        const std::string& active_peer,
        bool is_connected,
        const std::vector<std::pair<std::string, std::string>>& contacts,
        const std::vector<ChatBubble>& history) const noexcept;
};

} // namespace wire::ui

#endif // WIRE_IMESSAGE_TUI_HPP
