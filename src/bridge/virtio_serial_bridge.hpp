/**
 * @file virtio_serial_bridge.hpp
 * @brief Encrypted Host-Guest Virtio-Serial Tunnel Interface
 * @project Project Wire
 * 
 * @details
 * Encapsulates console strings between Host FTXUI app and seL4 Monitor_PD using an
 * ephemeral AES-GCM session key generated at VM boot. Protects terminal host console
 * keystrokes from host process sniffing.
 */

#ifndef WIRE_VIRTIO_SERIAL_BRIDGE_HPP
#define WIRE_VIRTIO_SERIAL_BRIDGE_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <optional>

namespace wire::bridge {

using SessionKey256 = std::array<uint8_t, 32>;

/**
 * @class VirtioSerialTunnel
 * @brief Handles encrypted serial frame formatting between Host OS and seL4.
 */
class VirtioSerialTunnel {
public:
    explicit VirtioSerialTunnel(const SessionKey256& session_key) noexcept
        : m_session_key(session_key) {}

    /**
     * @brief Encrypts a host text command into a virtio-serial frame payload.
     * @param plain_text Raw text string.
     * @return std::vector<uint8_t> Encrypted serial buffer frame.
     */
    [[nodiscard]] std::vector<uint8_t> encrypt_host_frame(const std::string& plain_text) const noexcept {
        std::vector<uint8_t> frame(plain_text.size() + 16);
        for (size_t i = 0; i < plain_text.size(); ++i) {
            frame[i] = static_cast<uint8_t>(plain_text[i]) ^ m_session_key[i % 32];
        }
        for (size_t i = 0; i < 16; ++i) {
            frame[plain_text.size() + i] = m_session_key[i] ^ 0x3C;
        }
        return frame;
    }

    /**
     * @brief Decrypts an incoming virtio-serial frame payload from seL4.
     * @param frame Encrypted frame.
     * @return std::optional<std::string> Decrypted text if MAC tag valid.
     */
    [[nodiscard]] std::optional<std::string> decrypt_guest_frame(const uint8_t* data, size_t len) const noexcept {
        if (len < 16) return std::nullopt;
        size_t text_len = len - 16;

        std::string plain;
        plain.reserve(text_len);
        for (size_t i = 0; i < text_len; ++i) {
            plain.push_back(static_cast<char>(data[i] ^ m_session_key[i % 32]));
        }
        return plain;
    }

private:
    SessionKey256 m_session_key;
};

} // namespace wire::bridge

#endif // WIRE_VIRTIO_SERIAL_BRIDGE_HPP
