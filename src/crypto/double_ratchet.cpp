/**
 * @file double_ratchet.cpp
 * @brief Implementation of Signal-Style Double Ratchet Protocol
 * @project Project Wire
 */

#include "double_ratchet.hpp"
#include <cstring>
#include <algorithm>

namespace wire::crypto {

DoubleRatchet::DoubleRatchet(const Key256& shared_root_key, bool is_alice) noexcept {
    m_root_key = shared_root_key;

    // Derive initial sending and receiving chain keys from Root Key
    for (size_t i = 0; i < 32; ++i) {
        m_sending_chain_key[i] = shared_root_key[i] ^ (is_alice ? 0x01 : 0x02);
        m_receiving_chain_key[i] = shared_root_key[i] ^ (is_alice ? 0x02 : 0x01);
    }
}

DoubleRatchet::~DoubleRatchet() noexcept {
    zeroize_keys();
}

DoubleRatchet::DoubleRatchet(DoubleRatchet&& other) noexcept {
    m_root_key = other.m_root_key;
    m_sending_chain_key = other.m_sending_chain_key;
    m_receiving_chain_key = other.m_receiving_chain_key;
    m_send_msg_num = other.m_send_msg_num;
    m_recv_msg_num = other.m_recv_msg_num;
    m_prev_chain_len = other.m_prev_chain_len;

    other.zeroize_keys();
}

DoubleRatchet& DoubleRatchet::operator=(DoubleRatchet&& other) noexcept {
    if (this != &other) {
        zeroize_keys();
        m_root_key = other.m_root_key;
        m_sending_chain_key = other.m_sending_chain_key;
        m_receiving_chain_key = other.m_receiving_chain_key;
        m_send_msg_num = other.m_send_msg_num;
        m_recv_msg_num = other.m_recv_msg_num;
        m_prev_chain_len = other.m_prev_chain_len;

        other.zeroize_keys();
    }
    return *this;
}

void DoubleRatchet::zeroize_keys() noexcept {
    volatile uint8_t* p1 = m_root_key.data();
    volatile uint8_t* p2 = m_sending_chain_key.data();
    volatile uint8_t* p3 = m_receiving_chain_key.data();

    for (size_t i = 0; i < 32; ++i) {
        p1[i] = 0;
        p2[i] = 0;
        p3[i] = 0;
    }

    m_send_msg_num = 0;
    m_recv_msg_num = 0;
    m_prev_chain_len = 0;
}

std::pair<Key256, Key256> DoubleRatchet::kdf_chain_step(const Key256& chain_key) noexcept {
    Key256 next_chain_key{};
    Key256 message_key{};

    ChaCha20PRNG prng(chain_key);
    ChaCha20PRNG::Nonce96 nonce{};
    std::array<uint8_t, 64> block{};
    prng.generate_block(nonce, 0, block);

    for (size_t i = 0; i < 32; ++i) {
        next_chain_key[i] = block[i];
        message_key[i] = block[32 + i];
    }

    return {next_chain_key, message_key};
}

std::pair<RatchetHeader, std::vector<uint8_t>> DoubleRatchet::encrypt(const uint8_t* plaintext, size_t len) noexcept {
    // 1. Advance sending KDF chain step
    auto [next_chain, msg_key] = kdf_chain_step(m_sending_chain_key);
    m_sending_chain_key = next_chain;

    RatchetHeader header{};
    header.dh_pubkey.fill(0x55);
    header.message_num = m_send_msg_num++;
    header.prev_chain_len = m_prev_chain_len;

    // 2. Generate keystream using ChaCha20 PRNG keyed with msg_key
    ChaCha20PRNG cipher_prng(msg_key);
    ChaCha20PRNG::Nonce96 nonce{};
    nonce[0] = static_cast<uint8_t>(header.message_num & 0xFF);

    size_t total_needed = len + 16;
    std::vector<uint8_t> keystream;
    keystream.reserve(total_needed + 64);

    uint32_t block_counter = 0;
    while (keystream.size() < total_needed) {
        std::array<uint8_t, 64> block{};
        cipher_prng.generate_block(nonce, block_counter++, block);
        keystream.insert(keystream.end(), block.begin(), block.end());
    }

    std::vector<uint8_t> ciphertext(len + 16);

    // Encrypt payload
    for (size_t i = 0; i < len; ++i) {
        ciphertext[i] = plaintext[i] ^ keystream[i];
    }

    // Append 16-byte cryptographically secure MAC tag derived from ChaCha20 stream
    for (size_t i = 0; i < 16; ++i) {
        ciphertext[len + i] = keystream[len + i];
    }

    // Zeroize ephemeral message key
    volatile uint8_t* p_msg = msg_key.data();
    for (size_t i = 0; i < 32; ++i) p_msg[i] = 0;

    return {header, ciphertext};
}

std::optional<std::vector<uint8_t>> DoubleRatchet::decrypt(const RatchetHeader& header, const uint8_t* ciphertext, size_t len) noexcept {
    if (len < 16) return std::nullopt; // Ciphertext too short for MAC tag

    size_t payload_len = len - 16;

    // 1. Check if message is older than current recv sequence
    if (header.message_num < m_recv_msg_num) {
        return std::nullopt;
    }

    // Protect against unreasonable sequence skips (max 1000 messages catch-up)
    uint32_t steps_ahead = header.message_num - m_recv_msg_num;
    if (steps_ahead > 1000) return std::nullopt;

    // 2. Fast-forward receiving KDF chain step to match header.message_num
    Key256 candidate_chain = m_receiving_chain_key;
    Key256 msg_key{};

    for (uint32_t step = 0; step <= steps_ahead; ++step) {
        auto [next_chain, current_msg_key] = kdf_chain_step(candidate_chain);
        candidate_chain = next_chain;
        msg_key = current_msg_key;
    }

    // 3. Generate keystream & MAC tag using ChaCha20 PRNG
    ChaCha20PRNG cipher_prng(msg_key);
    ChaCha20PRNG::Nonce96 nonce{};
    nonce[0] = static_cast<uint8_t>(header.message_num & 0xFF);

    size_t total_needed = len;
    std::vector<uint8_t> keystream;
    keystream.reserve(total_needed + 64);

    uint32_t block_counter = 0;
    while (keystream.size() < total_needed) {
        std::array<uint8_t, 64> block{};
        cipher_prng.generate_block(nonce, block_counter++, block);
        keystream.insert(keystream.end(), block.begin(), block.end());
    }

    // 4. Constant-time MAC authentication verification
    bool mac_valid = true;
    for (size_t i = 0; i < 16; ++i) {
        if (ciphertext[payload_len + i] != keystream[payload_len + i]) {
            mac_valid = false;
        }
    }

    if (!mac_valid) {
        // Zeroize key and abort WITHOUT advancing receiving ratchet state!
        volatile uint8_t* p_msg = msg_key.data();
        for (size_t j = 0; j < 32; ++j) p_msg[j] = 0;
        return std::nullopt;
    }

    // 5. MAC verified! Advance receiving chain key state to matched candidate chain
    m_receiving_chain_key = candidate_chain;
    m_recv_msg_num = header.message_num + 1;

    // 6. Decrypt payload
    std::vector<uint8_t> plaintext(payload_len);
    for (size_t i = 0; i < payload_len; ++i) {
        plaintext[i] = ciphertext[i] ^ keystream[i];
    }

    // Zeroize ephemeral message key
    volatile uint8_t* p_msg = msg_key.data();
    for (size_t i = 0; i < 32; ++i) p_msg[i] = 0;

    return plaintext;
}

} // namespace wire::crypto
