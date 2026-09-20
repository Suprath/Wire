/**
 * @file genesis_manager.hpp
 * @brief Genesis Entanglement Provisioning Engine (QR Payload & Armored Text Key File)
 * @project Project Wire
 * 
 * @details
 * Handles the creation, serialization, and deserialization of Genesis Entanglement Data.
 * Supports dual provisioning formats:
 * 1. Genesis Armored Base64 Key File (`wire_genesis.key`) for camera-less/headless hardware.
 * 2. Genesis Compact QR Code Data Payload.
 * 
 * Genesis Payload Contents:
 * - 256-bit Master Secret Seed
 * - 256-bit Peer Identity Public Key
 * - 64-bit Epoch Baseline Timestamp (T0)
 * - Candidate IPv6 Subnet Prefixes (/48 or /64)
 */

#ifndef WIRE_GENESIS_MANAGER_HPP
#define WIRE_GENESIS_MANAGER_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include "../crypto/chacha20_prng.hpp"

namespace wire::genesis {

using Key256 = std::array<uint8_t, 32>;

/**
 * @struct GenesisPayload
 * @brief Structure containing shared secret parameters generated during physical entanglement.
 */
struct GenesisPayload {
    Key256 master_seed;             /**< 256-bit shared Master Secret Seed */
    Key256 peer_identity_pubkey;    /**< 256-bit Peer Identity Public Key */
    uint64_t t0_timestamp;          /**< Baseline epoch timestamp T0 (seconds since UNIX epoch) */
    uint64_t primary_ipv6_prefix;   /**< Primary IPv6 /64 prefix (e.g. 0x20010db800000000) */

    /**
     * @brief Serializes the Genesis payload into a binary byte vector.
     * @return std::vector<uint8_t> Serialized byte buffer.
     */
    [[nodiscard]] std::vector<uint8_t> serialize_binary() const noexcept;

    /**
     * @brief Deserializes a binary byte buffer into a GenesisPayload structure.
     * @param buffer Raw byte buffer.
     * @return std::optional<GenesisPayload> Struct if valid, nullopt if malformed.
     */
    [[nodiscard]] static std::optional<GenesisPayload> deserialize_binary(const uint8_t* data, size_t len) noexcept;
};

/**
 * @class GenesisManager
 * @brief Manages export and import of Genesis QR code payloads and Armored Key files.
 */
class GenesisManager {
public:
    /**
     * @brief Creates a fresh Genesis Entanglement payload given a master seed and public key.
     * @param master_seed 256-bit master seed.
     * @param peer_pubkey 256-bit peer public key.
     * @param t0_timestamp Baseline epoch timestamp.
     * @param ipv6_prefix Target IPv6 prefix.
     * @return GenesisPayload Struct.
     */
    [[nodiscard]] static GenesisPayload create_genesis(
        const Key256& master_seed,
        const Key256& peer_pubkey,
        uint64_t t0_timestamp,
        uint64_t ipv6_prefix = 0x20010db800000000ULL) noexcept;

    /**
     * @brief Exports GenesisPayload into an Armored Base64 Text Key File string (`wire_genesis.key`).
     * @param payload Genesis payload.
     * @return std::string Armored text block.
     */
    [[nodiscard]] static std::string export_armored_key_file(const GenesisPayload& payload) noexcept;

    /**
     * @brief Imports and parses an Armored Base64 Text Key File string (`wire_genesis.key`).
     * @param armored_text Armored text block content.
     * @return std::optional<GenesisPayload> Genesis payload if valid, nullopt if invalid header or MAC tag.
     */
    [[nodiscard]] static std::optional<GenesisPayload> import_armored_key_file(const std::string& armored_text) noexcept;

    /**
     * @brief Encodes payload into a Base64 string format suitable for QR code scanning.
     * @param payload Genesis payload.
     * @return std::string Base64 string.
     */
    [[nodiscard]] static std::string export_qr_payload(const GenesisPayload& payload) noexcept;

private:
    [[nodiscard]] static std::string base64_encode(const uint8_t* data, size_t len) noexcept;
    [[nodiscard]] static std::vector<uint8_t> base64_decode(const std::string& input) noexcept;
};

} // namespace wire::genesis

#endif // WIRE_GENESIS_MANAGER_HPP
