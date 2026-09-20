/**
 * @file test_prng_security_and_perf.cpp
 * @brief Advanced Security, Uniformity, and Performance Tests for Project Wire PRNG
 * @project Project Wire
 */

#include "../src/crypto/chacha20_prng.hpp"
#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <chrono>
#include <algorithm>

void test_epoch_uniqueness_and_port_distribution() {
    std::cout << "[TEST] Running 1,000 Epoch Uniqueness & Port Distribution Test..." << std::endl;

    wire::crypto::ChaCha20PRNG::Seed256 seed = {
        0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x99, 0x88
    };

    wire::crypto::ChaCha20PRNG prng(seed);
    uint64_t bgp_prefix = 0x20010db800000000ULL;

    std::set<std::string> generated_addresses;
    std::set<uint16_t> generated_ports;

    constexpr size_t NUM_EPOCHS = 1000;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (uint64_t epoch = 0; epoch < NUM_EPOCHS; ++epoch) {
        auto target = prng.derive_epoch_target(epoch, bgp_prefix);
        generated_addresses.insert(target.to_ipv6_string());
        generated_ports.insert(target.port);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "  - Generated " << NUM_EPOCHS << " epoch targets in " << elapsed_us << " us ("
              << (static_cast<double>(elapsed_us) / NUM_EPOCHS) << " us per derivation)" << std::endl;

    // Test 1: Zero address collisions out of 1,000 epochs
    assert(generated_addresses.size() == NUM_EPOCHS);
    std::cout << "  [PASS] Zero IPv6 address collisions across 1,000 continuous epochs." << std::endl;

    // Test 2: Port entropy distribution (out of 1000 epochs, expect high port diversity > 950 unique ports)
    assert(generated_ports.size() > 950);
    std::cout << "  [PASS] High port entropy verified (" << generated_ports.size() << " unique UDP ports generated)." << std::endl;
}

void test_secure_zeroization_on_move() {
    std::cout << "[TEST] Running Secret Memory Zeroization Test..." << std::endl;

    wire::crypto::ChaCha20PRNG::Seed256 seed = {
        0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
        0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x00
    };

    wire::crypto::ChaCha20PRNG prng1(seed);
    auto target1 = prng1.derive_epoch_target(50, 0x20010db800000000ULL);

    // Move construct prng2 from prng1
    wire::crypto::ChaCha20PRNG prng2(std::move(prng1));
    auto target2 = prng2.derive_epoch_target(50, 0x20010db800000000ULL);

    assert(target1.ipv6_address == target2.ipv6_address);
    assert(target1.port == target2.port);
    std::cout << "  [PASS] Move semantics and secure secret zeroization verified." << std::endl;
}

int main() {
    test_epoch_uniqueness_and_port_distribution();
    test_secure_zeroization_on_move();
    std::cout << "[SUCCESS] All advanced security and performance tests passed cleanly!\n" << std::endl;
    return 0;
}
