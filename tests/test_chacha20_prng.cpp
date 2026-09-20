/**
 * @file test_chacha20_prng.cpp
 * @brief Unit tests for ChaCha20 PRNG predictive discovery calculations
 * @project Project Wire
 */

#include "../src/crypto/chacha20_prng.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

void test_chacha20_predictive_derivation() {
    std::cout << "[TEST] Running ChaCha20 Predictive Discovery Vector Test..." << std::endl;

    // Test Master Seed Vector
    wire::crypto::ChaCha20PRNG::Seed256 test_seed = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    wire::crypto::ChaCha20PRNG prng(test_seed);

    uint64_t epoch = 1000;
    uint64_t bgp_prefix_64 = 0x20010db800000000ULL; // 2001:db8::/64

    auto target_epoch1 = prng.derive_epoch_target(epoch, bgp_prefix_64);
    auto target_epoch2 = prng.derive_epoch_target(epoch, bgp_prefix_64);

    std::cout << "  - Epoch " << epoch << " Target IPv6: " << target_epoch1.to_ipv6_string() << std::endl;
    std::cout << "  - Epoch " << epoch << " Target Port: " << target_epoch1.port << std::endl;

    // Test 1: Determinism check (same seed + epoch must yield identical target)
    assert(target_epoch1.ipv6_address == target_epoch2.ipv6_address);
    assert(target_epoch1.port == target_epoch2.port);
    assert(target_epoch1.knock_key == target_epoch2.knock_key);
    std::cout << "  [PASS] Determinism test verified." << std::endl;

    // Test 2: Port range check (must be within 1024..65535)
    assert(target_epoch1.port >= 1024 && target_epoch1.port <= 65535);
    std::cout << "  [PASS] Port boundary test verified." << std::endl;

    // Test 3: Epoch change check (epoch + 1 must produce different target)
    auto target_epoch_next = prng.derive_epoch_target(epoch + 1, bgp_prefix_64);
    assert(target_epoch1.ipv6_address != target_epoch_next.ipv6_address);
    assert(target_epoch1.port != target_epoch_next.port || target_epoch1.ipv6_address != target_epoch_next.ipv6_address);
    std::cout << "  [PASS] Epoch hopping variance test verified." << std::endl;

    std::cout << "[SUCCESS] All ChaCha20 PRNG unit tests passed successfully!\n" << std::endl;
}

int main() {
    test_chacha20_predictive_derivation();
    return 0;
}
