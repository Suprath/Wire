/**
 * @file tls_mimicry.hpp
 * @brief HTTPS / Port-443 TLS 1.3 Record Frame Obfuscation & Mimicry Wrapper
 * @project Project Wire
 * 
 * @details
 * Wraps raw mathematical knock packets inside valid TLS 1.3 ClientHello (0x16) and
 * Application Data (0x17) record layer formats. This ensures traffic is indistinguishable
 * from standard HTTPS web browsing over TCP/UDP Port 443, bypassing restrictive DPI filters.
 */

#ifndef WIRE_TLS_MIMICRY_HPP
#define WIRE_TLS_MIMICRY_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <optional>

namespace wire::network {

// TLS Record Layer Types
constexpr uint8_t TLS_RECORD_HANDSHAKE    = 0x16;
constexpr uint8_t TLS_RECORD_APPLICATION  = 0x17;
constexpr uint16_t TLS_VERSION_1_2        = 0x0303; // Legacy version field in TLS 1.3 record header

/**
 * @class TLSMimicryEngine
 * @brief Encapsulates and decapsulates wire packets inside TLS 1.3 record layer envelopes.
 */
class TLSMimicryEngine {
public:
    /**
     * @brief Wraps a raw payload buffer in a TLS 1.3 Application Data (0x17) record layer frame.
     * @param payload Raw network packet bytes to encapsulate.
     * @return std::vector<uint8_t> TLS-framed byte buffer.
     */
    [[nodiscard]] static std::vector<uint8_t> wrap_tls_application_data(const uint8_t* payload, size_t len) noexcept;

    /**
     * @brief Generates a fake TLS 1.3 ClientHello (0x16) handshake record layer packet.
     * @param server_name SNI hostname hint (e.g. "cdn.cloudflare.net").
     * @return std::vector<uint8_t> TLS ClientHello frame.
     */
    [[nodiscard]] static std::vector<uint8_t> generate_client_hello(const char* server_name = "cdn.cloudflare.net") noexcept;

    /**
     * @brief Decapsulates a TLS 1.3 record envelope to extract the inner raw payload.
     * @param tls_frame TLS 1.3 formatted record buffer.
     * @return std::optional<std::vector<uint8_t>> Unwrapped payload if valid TLS frame, nullopt if malformed.
     */
    [[nodiscard]] static std::optional<std::vector<uint8_t>> unwrap_tls_record(const uint8_t* tls_frame, size_t len) noexcept;
};

} // namespace wire::network

#endif // WIRE_TLS_MIMICRY_HPP
