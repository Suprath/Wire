/**
 * @file chacha20_prng.hpp
 * @brief Memory-Safe, Constant-Time ChaCha20-CTR Predictive Discovery PRNG Engine
 * @project Project Wire
 * 
 * @details
 * Implements a cryptographically secure, constant-time ChaCha20-CTR Pseudo-Random Number
 * Generator (PRNG) for predictive IP/Port hopping and mathematical knock generation.
 * 
 * Security Features:
 * - Constant-time quarter-round operations to mitigate timing side-channel attacks.
 * - Memory-safe std::array containers with bounds validation.
 * - Automatic zeroization of sensitive master seeds upon destruction (RAII secret handling).
 * - Zero external library overhead, optimized for seL4 bare-metal microkernel execution.
 */

#ifndef WIRE_CHACHA20_PRNG_HPP
#define WIRE_CHACHA20_PRNG_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <string_view>
#include <optional>

namespace wire::crypto {

/**
 * @struct EpochTarget
 * @brief Contains the calculated predictive destination attributes for a given epoch.
 */
struct EpochTarget {
    std::array<uint8_t, 16> ipv6_address; /**< 128-bit IPv6 destination address */
    uint16_t port;                        /**< Target UDP port (1024 - 65535) */
    std::array<uint8_t, 32> knock_key;   /**< 256-bit ephemeral knock authentication key */
    
    /**
     * @brief Formats the 128-bit IPv6 address into standard hexadecimal string notation (e.g. 2001:db8::1).
     * @return std::string Formatted IPv6 address string.
     */
    [[nodiscard]] std::string to_ipv6_string() const;
};

/**
 * @class ChaCha20PRNG
 * @brief Constant-time ChaCha20 implementation for Project Wire Predictive Discovery.
 */
class ChaCha20PRNG {
public:
    using Seed256 = std::array<uint8_t, 32>;
    using Nonce96 = std::array<uint8_t, 12>;

    /**
     * @brief Constructs a ChaCha20PRNG instance with a 256-bit Master Seed.
     * @param master_seed 32-byte shared master secret derived during Genesis QR/File exchange.
     */
    explicit ChaCha20PRNG(const Seed256& master_seed) noexcept;

    /**
     * @brief Destructor that securely zeroes out sensitive seed data from RAM.
     */
    ~ChaCha20PRNG() noexcept;

    // Disable copy constructors to prevent unintentional copying of cryptographic secrets in memory
    ChaCha20PRNG(const ChaCha20PRNG&) = delete;
    ChaCha20PRNG& operator=(const ChaCha20PRNG&) = delete;

    // Enable move semantics with secure zeroization of source object
    ChaCha20PRNG(ChaCha20PRNG&& other) noexcept;
    ChaCha20PRNG& operator=(ChaCha20PRNG&& other) noexcept;

    /**
     * @brief Derives the predictive IPv6 address, port, and knock key for a given 5-minute Epoch.
     * 
     * Mathematical Formula:
     * Entropy_t = ChaCha20_Block(MasterSeed, Nonce = Epoch || Subnet_Index, Counter = 0)
     * IPv6_Target = [BGP_Prefix_64] | [Entropy_t[0..7]]
     * Port_Target = 1024 + (Entropy_t[8..9] mod 64512)
     * Knock_Key   = Entropy_t[16..47]
     * 
     * @param epoch 64-bit Epoch index derived from unix_timestamp / 300.
     * @param bgp_prefix_64 64-bit top network prefix (from Genesis subnets or global BGP table).
     * @return EpochTarget Struct containing the predictive IPv6 address, UDP port, and knock key.
     */
    [[nodiscard]] EpochTarget derive_epoch_target(uint64_t epoch, uint64_t bgp_prefix_64) const noexcept;

    /**
     * @brief Generates a pseudo-random 64-byte block using ChaCha20 stream cipher logic.
     * 
     * @param nonce 96-bit unique nonce (combines epoch, subnet index, and stream direction).
     * @param counter 32-bit block counter.
     * @param output_block Output array of 64 bytes to receive generated pseudo-random keystream.
     */
    void generate_block(const Nonce96& nonce, uint32_t counter, std::array<uint8_t, 64>& output_block) const noexcept;

private:
    Seed256 m_master_seed; /**< 256-bit Master Secret Key */

    /**
     * @brief Constant-time 32-bit left rotation helper function.
     * @param v Value to rotate.
     * @param shift Bit shift count.
     * @return uint32_t Rotated result.
     */
    [[nodiscard]] static inline uint32_t rotl32(uint32_t v, int shift) noexcept {
        return (v << shift) | (v >> (32 - shift));
    }

    /**
     * @brief Standard ChaCha20 Quarter-Round operation.
     * Operates in constant-time on four 32-bit state words.
     */
    static void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) noexcept;

    /**
     * @brief Secure memory zeroization helper to prevent secret leakage in deallocated RAM.
     * @param ptr Pointer to memory buffer.
     * @param size Size in bytes.
     */
    static void secure_zero(void* ptr, size_t size) noexcept;
};

} // namespace wire::crypto

#endif // WIRE_CHACHA20_PRNG_HPP
