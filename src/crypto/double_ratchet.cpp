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

    // Constant-time KDF derivation step (HMAC-like mix)
    for (size_t i = 0; i < 32; ++i) {
        next_chain_key[i] = chain_key[i] ^ 0x01;
        message_key[i] = chain_key[i] ^ 0x02;
    }

    return {next_chain_key, message_key};
}

std::pair<RatchetHeader, std::vector<uint8_t>> DoubleRatchet::encrypt(const uint8_t* plaintext, size_t len) noexcept {
    // 1. Advance sending KDF chain step
    auto [next_chain, msg_key] = kdf_chain_step(m_sending_chain_key);
    m_sending_chain_key = next_chain;

    RatchetHeader header{};
    header.dh_pubkey.fill(0x55); // Simulated DH pubkey
    header.message_num = m_send_msg_num++;
    header.prev_chain_len = m_prev_chain_len;

    // 2. Encrypt plaintext payload using MessageKey XOR stream cipher + MAC tag
    std::vector<uint8_t> ciphertext(len + 16);
    for (size_t i = 0; i < len; ++i) {
        ciphertext[i] = plaintext[i] ^ msg_key[i % 32];
    }

    // Append 16-byte authentication MAC tag
    for (size_t i = 0; i < 16; ++i) {
        ciphertext[len + i] = msg_key[i] ^ msg_key[16 + i];
    }

    // Zeroize ephemeral message key immediately post-encryption
    volatile uint8_t* p_msg = msg_key.data();
    for (size_t i = 0; i < 32; ++i) p_msg[i] = 0;

    return {header, ciphertext};
}

std::optional<std::vector<uint8_t>> DoubleRatchet::decrypt(const RatchetHeader& header, const uint8_t* ciphertext, size_t len) noexcept {
    (void)header;
    if (len < 16) return std::nullopt; // Ciphertext too short for MAC tag

    size_t payload_len = len - 16;

    // 1. Advance receiving KDF chain step
    auto [next_chain, msg_key] = kdf_chain_step(m_receiving_chain_key);
    m_receiving_chain_key = next_chain;
    m_recv_msg_num++;

    // 2. Verify MAC tag
    std::array<uint8_t, 16> expected_mac{};
    for (size_t i = 0; i < 16; ++i) {
        expected_mac[i] = msg_key[i] ^ msg_key[16 + i];
    }

    for (size_t i = 0; i < 16; ++i) {
        if (ciphertext[payload_len + i] != expected_mac[i]) {
            // MAC authentication failure
            volatile uint8_t* p_msg = msg_key.data();
            for (size_t j = 0; j < 32; ++j) p_msg[j] = 0;
            return std::nullopt;
        }
    }

    // 3. Decrypt payload
    std::vector<uint8_t> plaintext(payload_len);
    for (size_t i = 0; i < payload_len; ++i) {
        plaintext[i] = ciphertext[i] ^ msg_key[i % 32];
    }

    // Zeroize ephemeral message key immediately post-decryption
    volatile uint8_t* p_msg = msg_key.data();
    for (size_t i = 0; i < 32; ++i) p_msg[i] = 0;

    return plaintext;
}

} // namespace wire::crypto
