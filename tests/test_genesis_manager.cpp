/**
 * @file test_genesis_manager.cpp
 * @brief Unit tests for Genesis Provisioning Manager (QR & Armored Key File)
 * @project Project Wire
 */

#include "../src/genesis/genesis_manager.hpp"
#include <iostream>
#include <cassert>
#include <string>

void test_genesis_export_and_import() {
    std::cout << "[TEST] Running Genesis Provisioning Engine Test..." << std::endl;

    wire::genesis::Key256 seed{};
    seed.fill(0x55);

    wire::genesis::Key256 pubkey{};
    pubkey.fill(0xAA);

    uint64_t t0 = 1700000000;
    uint64_t prefix = 0x20010db812340000ULL;

    // 1. Create Genesis Payload
    auto payload_out = wire::genesis::GenesisManager::create_genesis(seed, pubkey, t0, prefix);

    // 2. Export Armored Key File (`wire_genesis.key`)
    std::string armored = wire::genesis::GenesisManager::export_armored_key_file(payload_out);
    std::cout << "  - Generated Armored Genesis Key File:\n" << armored << std::endl;

    assert(armored.find("-----BEGIN PROJECT WIRE GENESIS KEY-----") != std::string::npos);
    assert(armored.find("-----END PROJECT WIRE GENESIS KEY-----") != std::string::npos);

    // 3. Import and Parse Armored Key File
    auto payload_in = wire::genesis::GenesisManager::import_armored_key_file(armored);
    assert(payload_in.has_value());

    assert(payload_in->master_seed == seed);
    assert(payload_in->peer_identity_pubkey == pubkey);
    assert(payload_in->t0_timestamp == t0);
    assert(payload_in->primary_ipv6_prefix == prefix);

    std::cout << "  [PASS] Armored Key File export & import validation verified." << std::endl;

    // 4. Export Genesis QR Payload string
    std::string qr_payload = wire::genesis::GenesisManager::export_qr_payload(payload_out);
    assert(qr_payload.rfind("wire://", 0) == 0);
    std::cout << "  - QR Payload: " << qr_payload << std::endl;
    std::cout << "  [PASS] QR payload formatting verified." << std::endl;

    std::cout << "[SUCCESS] Genesis manager tests passed cleanly!\n" << std::endl;
}

int main() {
    test_genesis_export_and_import();
    return 0;
}
