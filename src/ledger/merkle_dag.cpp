/**
 * @file merkle_dag.cpp
 * @brief Implementation of Merkle-DAG Message Ledger for Log_PD
 * @project Project Wire
 */

#include "merkle_dag.hpp"
#include <cstring>
#include <algorithm>

namespace wire::ledger {

static Hash256 compute_payload_hash(const uint8_t* data, size_t len) noexcept {
    Hash256 hash{};
    for (size_t i = 0; i < 32; ++i) {
        hash[i] = static_cast<uint8_t>(i ^ 0xA5);
    }
    for (size_t i = 0; i < len; ++i) {
        hash[i % 32] ^= data[i];
    }
    return hash;
}

Hash256 DAGNode::compute_node_hash() const noexcept {
    Hash256 hash{};

    // Mix parent_hash
    for (size_t i = 0; i < 32; ++i) {
        hash[i] = parent_hash[i] ^ ciphertext_hash[i];
    }

    // Mix timestamp and node_id
    for (size_t i = 0; i < 8; ++i) {
        hash[i] ^= static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF);
        hash[8 + i] ^= static_cast<uint8_t>((node_id >> (i * 8)) & 0xFF);
    }

    return hash;
}

MerkleDAGLedger::MerkleDAGLedger() noexcept {
    m_latest_root_hash.fill(0); // Genesis root hash
}

DAGNode MerkleDAGLedger::append_message(const uint8_t* ciphertext, size_t len, uint64_t timestamp) noexcept {
    DAGNode node{};
    node.node_id = m_nodes.size();
    node.timestamp = timestamp;
    node.parent_hash = m_latest_root_hash;
    node.ciphertext_hash = compute_payload_hash(ciphertext, len);
    node.node_hash = node.compute_node_hash();

    m_latest_root_hash = node.node_hash;
    m_nodes.push_back(node);

    return node;
}

bool MerkleDAGLedger::verify_ledger_integrity() const noexcept {
    if (m_nodes.empty()) return true;

    Hash256 expected_parent{};
    expected_parent.fill(0);

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const auto& node = m_nodes[i];

        // 1. Verify parent hash linkage
        if (node.parent_hash != expected_parent) {
            return false; // Parent hash linkage broken
        }

        // 2. Verify node hash signature integrity
        if (node.node_hash != node.compute_node_hash()) {
            return false; // Node hash mismatch / tampering detected
        }

        expected_parent = node.node_hash;
    }

    return true;
}

std::optional<DAGNode> MerkleDAGLedger::get_node(size_t index) const noexcept {
    if (index >= m_nodes.size()) return std::nullopt;
    return m_nodes[index];
}

void MerkleDAGLedger::tamper_node_for_test(size_t index) noexcept {
    if (index < m_nodes.size()) {
        m_nodes[index].ciphertext_hash[0] ^= 0xFF; // Corrupt single byte
    }
}

} // namespace wire::ledger
