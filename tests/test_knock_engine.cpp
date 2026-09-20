/**
 * @file test_knock_engine.cpp
 * @brief Unit tests for 3-Packet Mathematical Knock Engine
 * @project Project Wire
 */

#include "../src/crypto/knock_engine.hpp"
#include <iostream>
#include <cassert>

void test_mathematical_knock_burst() {
    std::cout << "[TEST] Running 3-Packet Mathematical Knock Engine Test..." << std::endl;

    wire::crypto::ChaCha20PRNG::Seed256 seed = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };

    wire::crypto::ChaCha20PRNG prng(seed);
    auto target = prng.derive_epoch_target(500, 0x20010db800000000ULL);

    std::array<uint8_t, 32> sender_pubkey{};
    sender_pubkey.fill(0x77);

    uint64_t counter = 100;

    // 1. Generate 3-packet knock burst
    auto burst = wire::crypto::KnockEngine::generate_knock_burst(target, sender_pubkey, counter);
    assert(burst.size() == 3);

    std::cout << "  - Generated 3 knock packets. Verifying MTU padding & MAC tags..." << std::endl;

    for (size_t i = 0; i < 3; ++i) {
        auto wire_buffer = burst[i].serialize();
        assert(wire_buffer.size() == wire::crypto::KNOCK_MTU_PADDING_SIZE); // Must be 1280 bytes

        // Validate knock packet
        auto validated_pkt = wire::crypto::KnockEngine::validate_knock_packet(
            wire_buffer, target.knock_key, counter);

        assert(validated_pkt.has_value());
        assert(validated_pkt->packet_index == i);
        assert(validated_pkt->monotonic_counter == counter + i);
        assert(validated_pkt->sender_pubkey == sender_pubkey);
    }

    std::cout << "  [PASS] 3-packet knock burst generation & cryptographic validation verified." << std::endl;

    // 2. Replay protection test (attempting to validate an old counter must fail)
    auto wire_buffer_old = burst[0].serialize();
    auto replay_result = wire::crypto::KnockEngine::validate_knock_packet(
        wire_buffer_old, target.knock_key, counter + 50); // expected counter higher

    assert(!replay_result.has_value());
    std::cout << "  [PASS] Replay protection anti-counter check verified." << std::endl;

    std::cout << "[SUCCESS] Knock engine tests passed cleanly!\n" << std::endl;
}

int main() {
    test_mathematical_knock_burst();
    return 0;
}
