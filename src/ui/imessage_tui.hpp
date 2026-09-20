/**
 * @file imessage_tui.hpp
 * @brief Terminal UI/UX Engine for Project Wire (Genesis Entanglement & Chat Views)
 * @project Project Wire
 * 
 * @details
 * Formats clean, minimal Terminal UI screens:
 * - Genesis Setup Screen: Display local Armored Key File & QR string payload.
 * - Add Peer Screen: Paste peer key payload and assign custom local username alias.
 * - Main Chat Interface: iMessage-style split pane (Sidebar contact list + Speech bubbles).
 */

#ifndef WIRE_IMESSAGE_TUI_HPP
#define WIRE_IMESSAGE_TUI_HPP

#include <string>
#include <vector>

namespace wire::ui {

struct ChatBubble {
    std::string sender;    /**< "YOU" or peer display name */
    std::string message;   /**< Text content */
    std::string timestamp; /**< Formatted time (HH:MM) */
    bool is_outgoing;      /**< true if sent by local user (right-aligned cyan bubble) */
};

enum class TUIScreen {
    MAIN_MENU,          /**< Main Dashboard Menu */
    MY_GENESIS_KEY,     /**< Display local Genesis Armored Key & QR payload */
    ADD_PEER_CONTACT,   /**< Paste peer key payload & assign custom display name */
    CHAT_INTERFACE      /**< Active iMessage-style speech bubble chat pane */
};

class iMessageTUI {
public:
    iMessageTUI() = default;

    /**
     * @brief Renders the Main Dashboard & Entanglement Menu.
     * @param my_pubkey_hex Local 256-bit Identity Public Key Hash.
     * @param contact_count Number of saved peer contacts.
     */
    void render_main_menu(const std::string& my_pubkey_hex, size_t contact_count) const noexcept;

    /**
     * @brief Renders the user's own Genesis Entanglement Key & QR string payload screen.
     * @param armored_key_text Armored Base64 Key File string.
     * @param qr_payload Base64 QR code string payload.
     */
    void render_my_genesis_key(const std::string& armored_key_text, const std::string& qr_payload) const noexcept;

    /**
     * @brief Renders the Add Peer Entanglement prompt screen.
     */
    void render_add_peer_screen() const noexcept;

    /**
     * @brief Renders the minimal iMessage-style split-pane chat interface layout.
     * @param active_peer Active peer display name (e.g. "Bob").
     * @param is_connected Connection status.
     * @param contacts List of contact tuples (Name, StatusBadge).
     * @param history Conversation message bubbles.
     */
    void render_chat_layout(
        const std::string& active_peer,
        bool is_connected,
        const std::vector<std::pair<std::string, std::string>>& contacts,
        const std::vector<ChatBubble>& history) const noexcept;
};

} // namespace wire::ui

#endif // WIRE_IMESSAGE_TUI_HPP
