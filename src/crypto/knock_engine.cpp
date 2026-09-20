/**
 * @file knock_engine.cpp
 * @brief Implementation of 3-Packet Mathematical Knock Engine
 * @project Project Wire
 */

#include "knock_engine.hpp"
#include <cstring>
#include <algorithm>

namespace wire::crypto {

uint16_t KnockEngine::calculate_knock_length(const std::array<uint8_t, 32>& knock_key, uint8_t packet_index) noexcept {
    // Hash key byte + packet index offset
    uint32_t val = static_cast<uint32_t>(knock_key[packet_index * 4]) |
                   (static_cast<uint32_t>(knock_key[packet_index * 4 + 1]) << 8);
    return static_cast<uint16_t>(64 + (val % 1024));
}

std::array<uint8_t, KNOCK_MTU_PADDING_SIZE> KnockPacket::serialize() const noexcept {
    std::array<uint8_t, KNOCK_MTU_PADDING_SIZE> buffer{};

    // Header layout:
    // Byte 0: Packet Index (1 byte)
    // Bytes 1..8: Epoch (uint64_t)
    // Bytes 9..16: Monotonic Counter (uint64_t)
    // Bytes 17..48: Sender Public Key (32 bytes)
    // Bytes 49..64: MAC Tag (16 bytes)
    buffer[0] = packet_index;

    for (size_t i = 0; i < 8; ++i) {
        buffer[1 + i] = static_cast<uint8_t>((epoch >> (i * 8)) & 0xFF);
        buffer[9 + i] = static_cast<uint8_t>((monotonic_counter >> (i * 8)) & 0xFF);
    }

    std::copy(sender_pubkey.begin(), sender_pubkey.end(), buffer.begin() + 17);
    std::copy(mac_tag.begin(), mac_tag.end(), buffer.begin() + 49);

    // Fill remaining payload bytes up to 1280 bytes MTU padding
    size_t offset = 65;
    if (!payload_data.empty()) {
        size_t copy_len = std::min(payload_data.size(), KNOCK_MTU_PADDING_SIZE - offset);
        std::copy(payload_data.begin(), payload_data.begin() + static_cast<ptrdiff_t>(copy_len), buffer.begin() + static_cast<ptrdiff_t>(offset));
    }

    return buffer;
}

std::array<KnockPacket, KNOCK_BURST_COUNT> KnockEngine::generate_knock_burst(
    const EpochTarget& target,
    const std::array<uint8_t, 32>& sender_pubkey,
    uint64_t counter) noexcept {

    std::array<KnockPacket, KNOCK_BURST_COUNT> burst{};

    for (uint8_t idx = 0; idx < KNOCK_BURST_COUNT; ++idx) {
        KnockPacket pkt{};
        pkt.packet_index = idx;
        pkt.epoch = 0; // Filled from target if needed
        pkt.monotonic_counter = counter + idx;
        pkt.sender_pubkey = sender_pubkey;

        // Compute MAC tag derived from target.knock_key
        for (size_t i = 0; i < 16; ++i) {
            pkt.mac_tag[i] = target.knock_key[i] ^ target.knock_key[16 + i] ^ idx;
        }

        uint16_t target_len = calculate_knock_length(target.knock_key, idx);
        pkt.payload_data.resize(target_len, 0xA5); // Fill deterministic pattern

        burst[idx] = std::move(pkt);
    }

    return burst;
}

std::optional<KnockPacket> KnockEngine::validate_knock_packet(
    const std::array<uint8_t, KNOCK_MTU_PADDING_SIZE>& buffer,
    const std::array<uint8_t, 32>& knock_key,
    uint64_t current_counter) noexcept {

    KnockPacket pkt{};
    pkt.packet_index = buffer[0];

    pkt.epoch = 0;
    pkt.monotonic_counter = 0;
    for (size_t i = 0; i < 8; ++i) {
        pkt.epoch |= (static_cast<uint64_t>(buffer[1 + i]) << (i * 8));
        pkt.monotonic_counter |= (static_cast<uint64_t>(buffer[9 + i]) << (i * 8));
    }

    // Monotonic anti-replay counter check
    if (pkt.monotonic_counter < current_counter) {
        return std::nullopt; // Replay attack detected
    }

    std::copy(buffer.begin() + 17, buffer.begin() + 49, pkt.sender_pubkey.begin());
    std::copy(buffer.begin() + 49, buffer.begin() + 65, pkt.mac_tag.begin());

    // Verify MAC tag
    std::array<uint8_t, 16> expected_mac{};
    for (size_t i = 0; i < 16; ++i) {
        expected_mac[i] = knock_key[i] ^ knock_key[16 + i] ^ pkt.packet_index;
    }

    if (pkt.mac_tag != expected_mac) {
        return std::nullopt; // Invalid MAC tag authentication
    }

    return pkt;
}

} // namespace wire::crypto
