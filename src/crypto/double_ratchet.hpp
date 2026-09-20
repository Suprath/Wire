/**
 * @file double_ratchet.hpp
 * @brief Signal-Style Double Ratchet Protocol Engine for Project Wire
 * @project Project Wire
 * 
 * @details
 * Implements the Double Ratchet Algorithm combining Symmetric KDF Chain Ratcheting
 * and Diffie-Hellman Ephemeral Key Ratcheting over ChaCha20-Poly1305 / HKDF.
 * 
 * Security Guarantees:
 * - Perfect Forward Secrecy (PFS): Past messages cannot be decrypted if current keys are compromised.
 * - Break-in Recovery: Future messages regain secrecy even after a temporary key compromise.
 * - Automatic Key Zeroization: Ephemeral message keys are wiped immediately post-encryption/decryption.
 */

#ifndef WIRE_DOUBLE_RATCHET_HPP
#define WIRE_DOUBLE_RATCHET_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <optional>
#include "chacha20_prng.hpp"

namespace wire::crypto {

using Key256 = std::array<uint8_t, 32>;

/**
 * @struct RatchetHeader
 * @brief Header attached to every Double Ratchet encrypted message payload.
 */
struct RatchetHeader {
    Key256 dh_pubkey;       /**< Ephemeral DH public key of sender */
    uint32_t message_num;   /**< Message index in current sending chain */
    uint32_t prev_chain_len;/**< Length of previous sending chain */
};

/**
 * @class DoubleRatchet
 * @brief Signal-Style Double Ratchet Protocol implementation inside Vault_PD.
 */
class DoubleRatchet {
public:
    /**
     * @brief Constructs a DoubleRatchet instance initialized with shared Root Key.
     * @param shared_root_key 256-bit master root key (derived from Noise_KK / Genesis Seed).
     * @param is_alice true if Alice (initiator), false if Bob (responder).
     */
    explicit DoubleRatchet(const Key256& shared_root_key, bool is_alice) noexcept;

    /**
     * @brief Destructor that securely zeroes out all root, chain, and message keys in RAM.
     */
    ~DoubleRatchet() noexcept;

    // Prevent copying of sensitive ratchet state
    DoubleRatchet(const DoubleRatchet&) = delete;
    DoubleRatchet& operator=(const DoubleRatchet&) = delete;

    // Allow move semantics with secure zeroization
    DoubleRatchet(DoubleRatchet&& other) noexcept;
    DoubleRatchet& operator=(DoubleRatchet&& other) noexcept;

    /**
     * @brief Encrypts a plaintext message payload using the current sending ratchet.
     * @param plaintext Raw message bytes to encrypt.
     * @return std::pair<RatchetHeader, std::vector<uint8_t>> Tuple of RatchetHeader and ciphertext.
     */
    [[nodiscard]] std::pair<RatchetHeader, std::vector<uint8_t>> encrypt(const uint8_t* plaintext, size_t len) noexcept;

    /**
     * @brief Decrypts a ciphertext payload using the receiving ratchet.
     * @param header Message header attached to ciphertext.
     * @param ciphertext Encrypted payload bytes.
     * @return std::optional<std::vector<uint8_t>> Plaintext bytes if MAC authentication passes, nullopt if invalid.
     */
    [[nodiscard]] std::optional<std::vector<uint8_t>> decrypt(const RatchetHeader& header, const uint8_t* ciphertext, size_t len) noexcept;

private:
    Key256 m_root_key;           /**< 256-bit current Root Key */
    Key256 m_sending_chain_key;  /**< 256-bit current Sending Chain Key */
    Key256 m_receiving_chain_key;/**< 256-bit current Receiving Chain Key */
    uint32_t m_send_msg_num{0};  /**< Message counter for sending chain */
    uint32_t m_recv_msg_num{0};  /**< Message counter for receiving chain */
    uint32_t m_prev_chain_len{0};/**< Length of previous sending chain */

    /**
     * @brief KDF Chain Step deriving next ChainKey and MessageKey.
     */
    static std::pair<Key256, Key256> kdf_chain_step(const Key256& chain_key) noexcept;

    /**
     * @brief Secure memory zeroization helper.
     */
    void zeroize_keys() noexcept;
};

} // namespace wire::crypto

#endif // WIRE_DOUBLE_RATCHET_HPP
