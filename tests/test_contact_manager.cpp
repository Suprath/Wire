/**
 * @file test_contact_manager.cpp
 * @brief Unit tests for Multi-Peer Contact Manager & Custom Alias Store
 * @project Project Wire
 */

#include "../src/contact/contact_manager.hpp"
#include <iostream>
#include <cassert>
#include <string>

void test_multi_peer_contact_management() {
    std::cout << "[TEST] Running Multi-Peer Contact Vault & Custom Username Test..." << std::endl;

    wire::contact::ContactManager vault;

    // 1. Create Genesis Payloads for 3 distinct contacts
    wire::genesis::Key256 seed1{}; seed1.fill(0x11);
    wire::genesis::Key256 pubkey1{}; pubkey1.fill(0xAA);
    auto payload_alice = wire::genesis::GenesisManager::create_genesis(seed1, pubkey1, 1000);

    wire::genesis::Key256 seed2{}; seed2.fill(0x22);
    wire::genesis::Key256 pubkey2{}; pubkey2.fill(0xBB);
    auto payload_bob = wire::genesis::GenesisManager::create_genesis(seed2, pubkey2, 1000);

    wire::genesis::Key256 seed3{}; seed3.fill(0x33);
    wire::genesis::Key256 pubkey3{}; pubkey3.fill(0xCC);
    auto payload_charlie = wire::genesis::GenesisManager::create_genesis(seed3, pubkey3, 1000);

    // 2. Register contacts with custom local display aliases
    std::string id_alice = vault.add_contact("Alice (Work)", payload_alice);
    std::string id_bob = vault.add_contact("Bob", payload_bob);

    // Register Charlie using Armored Key File import
    std::string charlie_armored = wire::genesis::GenesisManager::export_armored_key_file(payload_charlie);
    auto id_charlie = vault.add_contact_from_armored_file("Dr. Charlie", charlie_armored);
    assert(id_charlie.has_value());

    assert(vault.contact_count() == 3);
    std::cout << "  - Registered 3 contacts with custom local display aliases ('Alice (Work)', 'Bob', 'Dr. Charlie')." << std::endl;

    // 3. Retrieve contacts by local alias
    auto* contact_alice = vault.find_contact_by_alias("Alice (Work)");
    auto* contact_bob = vault.find_contact_by_alias("bob"); // Case-insensitive lookup
    auto* contact_charlie = vault.find_contact_by_alias("Dr. Charlie");

    assert(contact_alice != nullptr);
    assert(contact_bob != nullptr);
    assert(contact_charlie != nullptr);
    std::cout << "  [PASS] Custom local username lookup verified." << std::endl;

    // 4. Test concurrent, isolated chat sessions
    std::cout << "  - Testing concurrent isolated messaging with Alice, Bob, and Charlie..." << std::endl;

    // Send message to Alice
    std::string msg_alice = "Hey Alice, reviewing the seL4 Vault architecture.";
    auto [hdr_a, cipher_a] = contact_alice->ratchet->encrypt(
        reinterpret_cast<const uint8_t*>(msg_alice.data()), msg_alice.size());
    contact_alice->ledger->append_message(cipher_a.data(), cipher_a.size(), 1000);

    // Send message to Bob
    std::string msg_bob = "Bob, knock engine is online.";
    auto [hdr_b, cipher_b] = contact_bob->ratchet->encrypt(
        reinterpret_cast<const uint8_t*>(msg_bob.data()), msg_bob.size());
    contact_bob->ledger->append_message(cipher_b.data(), cipher_b.size(), 1001);

    // Send message to Charlie
    std::string msg_charlie = "Dr. Charlie, TLS mimicry tests passed 100%.";
    auto [hdr_c, cipher_c] = contact_charlie->ratchet->encrypt(
        reinterpret_cast<const uint8_t*>(msg_charlie.data()), msg_charlie.size());
    contact_charlie->ledger->append_message(cipher_c.data(), cipher_c.size(), 1002);

    // Verify independent DAG ledger integrity for all 3 chats
    assert(contact_alice->ledger->size() == 1);
    assert(contact_bob->ledger->size() == 1);
    assert(contact_charlie->ledger->size() == 1);

    assert(contact_alice->ledger->verify_ledger_integrity() == true);
    assert(contact_bob->ledger->verify_ledger_integrity() == true);
    assert(contact_charlie->ledger->verify_ledger_integrity() == true);

    std::cout << "  [PASS] Concurrent multi-peer messaging & independent Merkle-DAG ledgers verified." << std::endl;

    // 5. Test discovery state independence
    contact_alice->discovery->start_on_demand_search();
    assert(contact_alice->discovery->current_state() == wire::network::PeerState::SEARCHING);
    assert(contact_bob->discovery->current_state() == wire::network::PeerState::IDLE);

    std::cout << "  [PASS] Independent on-demand search states per contact verified." << std::endl;

    std::cout << "[SUCCESS] Multi-peer contact manager tests passed cleanly!\n" << std::endl;
}

int main() {
    test_multi_peer_contact_management();
    return 0;
}
