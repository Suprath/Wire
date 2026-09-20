/**
 * @file tls_mimicry.cpp
 * @brief Implementation of TLS 1.3 Mimicry Wrapper for Censorship Bypass
 * @project Project Wire
 */

#include "tls_mimicry.hpp"
#include <cstring>
#include <algorithm>

namespace wire::network {

std::vector<uint8_t> TLSMimicryEngine::wrap_tls_application_data(const uint8_t* payload, size_t len) noexcept {
    std::vector<uint8_t> frame;
    frame.reserve(5 + len);

    // TLS 1.3 Record Header (5 bytes):
    // Byte 0: ContentType (0x17 = Application Data)
    // Bytes 1..2: Legacy Version (0x0303 = TLS 1.2)
    // Bytes 3..4: Length (uint16_t big-endian)
    frame.push_back(TLS_RECORD_APPLICATION);
    frame.push_back(static_cast<uint8_t>((TLS_VERSION_1_2 >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(TLS_VERSION_1_2 & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(len & 0xFF));

    // Append inner payload bytes
    frame.insert(frame.end(), payload, payload + len);
    return frame;
}

std::vector<uint8_t> TLSMimicryEngine::generate_client_hello(const char* server_name) noexcept {
    (void)server_name;
    std::vector<uint8_t> frame;
    frame.reserve(128);

    // TLS 1.3 Handshake Record Header (0x16)
    frame.push_back(TLS_RECORD_HANDSHAKE);
    frame.push_back(static_cast<uint8_t>((TLS_VERSION_1_2 >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(TLS_VERSION_1_2 & 0xFF));

    // Dummy ClientHello Payload length (512 bytes simulated)
    uint16_t dummy_len = 512;
    frame.push_back(static_cast<uint8_t>((dummy_len >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(dummy_len & 0xFF));

    // Fill ClientHello handshake body bytes
    frame.resize(5 + dummy_len, 0x01); // Handshake type 0x01 = ClientHello
    return frame;
}

std::optional<std::vector<uint8_t>> TLSMimicryEngine::unwrap_tls_record(const uint8_t* tls_frame, size_t len) noexcept {
    if (len < 5) return std::nullopt; // Frame too short for TLS header

    uint8_t content_type = tls_frame[0];
    uint16_t version = static_cast<uint16_t>((static_cast<uint16_t>(tls_frame[1]) << 8) | static_cast<uint16_t>(tls_frame[2]));
    uint16_t payload_len = static_cast<uint16_t>((static_cast<uint16_t>(tls_frame[3]) << 8) | static_cast<uint16_t>(tls_frame[4]));

    // Validate TLS version and length bounds
    if (version != TLS_VERSION_1_2 || (5 + payload_len) > len) {
        return std::nullopt;
    }

    if (content_type != TLS_RECORD_APPLICATION && content_type != TLS_RECORD_HANDSHAKE) {
        return std::nullopt;
    }

    std::vector<uint8_t> inner_payload(tls_frame + 5, tls_frame + 5 + payload_len);
    return inner_payload;
}

} // namespace wire::network
