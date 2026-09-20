/**
 * @file chacha20_prng.cpp
 * @brief Implementation of Constant-Time ChaCha20-CTR Predictive Discovery PRNG
 * @project Project Wire
 */

#include "chacha20_prng.hpp"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace wire::crypto {

// ChaCha20 Constant Constants ("expand 32-byte k")
static constexpr uint32_t CHACHA20_CONSTANTS[4] = {
    0x61707865, 0x33303232, 0x7962326d, 0x6b206574
};

std::string EpochTarget::to_ipv6_string() const {
    char buf[40];
    std::snprintf(buf, sizeof(buf),
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
        ipv6_address[0], ipv6_address[1], ipv6_address[2], ipv6_address[3],
        ipv6_address[4], ipv6_address[5], ipv6_address[6], ipv6_address[7],
        ipv6_address[8], ipv6_address[9], ipv6_address[10], ipv6_address[11],
        ipv6_address[12], ipv6_address[13], ipv6_address[14], ipv6_address[15]);
    return std::string(buf);
}

ChaCha20PRNG::ChaCha20PRNG(const Seed256& master_seed) noexcept {
    // Copy master seed into member variable safely
    std::copy(master_seed.begin(), master_seed.end(), m_master_seed.begin());
}

ChaCha20PRNG::~ChaCha20PRNG() noexcept {
    // Securely wipe memory on destruction to prevent seed remnants in RAM
    secure_zero(m_master_seed.data(), m_master_seed.size());
}

ChaCha20PRNG::ChaCha20PRNG(ChaCha20PRNG&& other) noexcept {
    std::copy(other.m_master_seed.begin(), other.m_master_seed.end(), m_master_seed.begin());
    secure_zero(other.m_master_seed.data(), other.m_master_seed.size());
}

ChaCha20PRNG& ChaCha20PRNG::operator=(ChaCha20PRNG&& other) noexcept {
    if (this != &other) {
        secure_zero(m_master_seed.data(), m_master_seed.size());
        std::copy(other.m_master_seed.begin(), other.m_master_seed.end(), m_master_seed.begin());
        secure_zero(other.m_master_seed.data(), other.m_master_seed.size());
    }
    return *this;
}

void ChaCha20PRNG::secure_zero(void* ptr, size_t size) noexcept {
    if (ptr == nullptr || size == 0) return;
    
    // Use volatile pointer to prevent compiler from optimizing away memory wiping
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    while (size--) {
        *p++ = 0;
    }
}

void ChaCha20PRNG::quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) noexcept {
    a += b; d ^= a; d = rotl32(d, 16);
    c += d; b ^= c; b = rotl32(b, 12);
    a += b; d ^= a; d = rotl32(d, 8);
    c += d; b ^= c; b = rotl32(b, 7);
}

void ChaCha20PRNG::generate_block(const Nonce96& nonce, uint32_t counter, std::array<uint8_t, 64>& output_block) const noexcept {
    std::array<uint32_t, 16> state{};

    // 1. Initialize ChaCha20 State Matrix (16 32-bit words)
    // Words 0-3: Constants "expand 32-byte k"
    state[0] = CHACHA20_CONSTANTS[0];
    state[1] = CHACHA20_CONSTANTS[1];
    state[2] = CHACHA20_CONSTANTS[2];
    state[3] = CHACHA20_CONSTANTS[3];

    // Words 4-11: 256-bit Master Key (Little-Endian)
    for (size_t i = 0; i < 8; ++i) {
        state[4 + i] = static_cast<uint32_t>(m_master_seed[i * 4]) |
                       (static_cast<uint32_t>(m_master_seed[i * 4 + 1]) << 8) |
                       (static_cast<uint32_t>(m_master_seed[i * 4 + 2]) << 16) |
                       (static_cast<uint32_t>(m_master_seed[i * 4 + 3]) << 24);
    }

    // Word 12: Block Counter
    state[12] = counter;

    // Words 13-15: 96-bit Nonce
    for (size_t i = 0; i < 3; ++i) {
        state[13 + i] = static_cast<uint32_t>(nonce[i * 4]) |
                        (static_cast<uint32_t>(nonce[i * 4 + 1]) << 8) |
                        (static_cast<uint32_t>(nonce[i * 4 + 2]) << 16) |
                        (static_cast<uint32_t>(nonce[i * 4 + 3]) << 24);
    }

    // Copy working state
    std::array<uint32_t, 16> working_state = state;

    // 2. Perform 20 Rounds (10 Column Rounds + 10 Diagonal Rounds)
    for (int i = 0; i < 10; ++i) {
        // Column rounds
        quarter_round(working_state[0], working_state[4], working_state[8],  working_state[12]);
        quarter_round(working_state[1], working_state[5], working_state[9],  working_state[13]);
        quarter_round(working_state[2], working_state[6], working_state[10], working_state[14]);
        quarter_round(working_state[3], working_state[7], working_state[11], working_state[15]);

        // Diagonal rounds
        quarter_round(working_state[0], working_state[5], working_state[10], working_state[15]);
        quarter_round(working_state[1], working_state[6], working_state[11], working_state[12]);
        quarter_round(working_state[2], working_state[7], working_state[8],  working_state[13]);
        quarter_round(working_state[3], working_state[4], working_state[9],  working_state[14]);
    }

    // 3. Add initial state to working state
    for (size_t i = 0; i < 16; ++i) {
        working_state[i] += state[i];
    }

    // 4. Serialize working state to output byte array (Little-Endian)
    for (size_t i = 0; i < 16; ++i) {
        output_block[i * 4 + 0] = static_cast<uint8_t>(working_state[i] & 0xFF);
        output_block[i * 4 + 1] = static_cast<uint8_t>((working_state[i] >> 8) & 0xFF);
        output_block[i * 4 + 2] = static_cast<uint8_t>((working_state[i] >> 16) & 0xFF);
        output_block[i * 4 + 3] = static_cast<uint8_t>((working_state[i] >> 24) & 0xFF);
    }

    // Wipe temporary working state
    secure_zero(working_state.data(), working_state.size() * sizeof(uint32_t));
    secure_zero(state.data(), state.size() * sizeof(uint32_t));
}

EpochTarget ChaCha20PRNG::derive_epoch_target(uint64_t epoch, uint64_t bgp_prefix_64) const noexcept {
    // 1. Construct 96-bit Nonce from 64-bit Epoch and 32-bit constant padding
    Nonce96 nonce{};
    nonce[0] = static_cast<uint8_t>(epoch & 0xFF);
    nonce[1] = static_cast<uint8_t>((epoch >> 8) & 0xFF);
    nonce[2] = static_cast<uint8_t>((epoch >> 16) & 0xFF);
    nonce[3] = static_cast<uint8_t>((epoch >> 24) & 0xFF);
    nonce[4] = static_cast<uint8_t>((epoch >> 32) & 0xFF);
    nonce[5] = static_cast<uint8_t>((epoch >> 40) & 0xFF);
    nonce[6] = static_cast<uint8_t>((epoch >> 48) & 0xFF);
    nonce[7] = static_cast<uint8_t>((epoch >> 56) & 0xFF);
    // bytes 8..11 reserved / zeroed

    // 2. Generate 64 pseudo-random bytes for target derivation
    std::array<uint8_t, 64> block{};
    generate_block(nonce, 0, block);

    EpochTarget target{};

    // 3. Construct IPv6 Address: Top 64 bits = BGP Prefix, Bottom 64 bits = PRNG Entropy
    for (size_t i = 0; i < 8; ++i) {
        target.ipv6_address[i] = static_cast<uint8_t>((bgp_prefix_64 >> ((7 - i) * 8)) & 0xFF);
    }
    for (size_t i = 0; i < 8; ++i) {
        target.ipv6_address[8 + i] = block[i];
    }

    // 4. Derive Target UDP Port (Range: 1024 to 65535)
    uint16_t raw_port = static_cast<uint16_t>(static_cast<uint16_t>(block[8]) | (static_cast<uint16_t>(block[9]) << 8));
    target.port = static_cast<uint16_t>(1024 + (raw_port % 64512));

    // 5. Derive 256-bit Ephemeral Knock Authentication Key
    std::copy(block.begin() + 16, block.begin() + 48, target.knock_key.begin());

    // Wipe block memory
    secure_zero(block.data(), block.size());

    return target;
}

} // namespace wire::crypto
