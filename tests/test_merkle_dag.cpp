/**
 * @file test_merkle_dag.cpp
 * @brief Unit tests for Immutable Merkle-DAG Ledger
 * @project Project Wire
 */

#include "../src/ledger/merkle_dag.hpp"
#include <iostream>
#include <cassert>
#include <string>

void test_merkle_dag_ledger_integrity() {
    std::cout << "[TEST] Running Merkle-DAG Immutable Ledger Test..." << std::endl;

    wire::ledger::MerkleDAGLedger ledger;

    std::string msg1 = "Ciphertext Block #1";
    std::string msg2 = "Ciphertext Block #2";
    std::string msg3 = "Ciphertext Block #3";

    // 1. Append 3 messages
    auto node1 = ledger.append_message(reinterpret_cast<const uint8_t*>(msg1.data()), msg1.size(), 1000);
    auto node2 = ledger.append_message(reinterpret_cast<const uint8_t*>(msg2.data()), msg2.size(), 1005);
    auto node3 = ledger.append_message(reinterpret_cast<const uint8_t*>(msg3.data()), msg3.size(), 1010);

    assert(ledger.size() == 3);
    std::cout << "  - Appended 3 DAG nodes. Verifying parent linkage..." << std::endl;

    // Verify parent linkage
    assert(node2.parent_hash == node1.node_hash);
    assert(node3.parent_hash == node2.node_hash);

    // 2. Verify ledger integrity
    assert(ledger.verify_ledger_integrity() == true);
    std::cout << "  [PASS] Merkle-DAG 100% cryptographic integrity verified." << std::endl;

    // 3. Test tamper detection (tamper node #1)
    std::cout << "  - Simulating illegal tampering on Node #1..." << std::endl;
    ledger.tamper_node_for_test(1);

    // Verify tamper detection trigger
    assert(ledger.verify_ledger_integrity() == false);
    std::cout << "  [PASS] Tamper detection triggered successfully! Ledger invalid state detected." << std::endl;

    std::cout << "[SUCCESS] Merkle-DAG ledger tests passed cleanly!\n" << std::endl;
}

int main() {
    test_merkle_dag_ledger_integrity();
    return 0;
}
