/**
 * @file merkle_dag.hpp
 * @brief Immutable Merkle-DAG Message Ledger for Project Wire Log_PD
 * @project Project Wire
 * 
 * @details
 * Implements an append-only Merkle-Directed Acyclic Graph (Merkle-DAG) ledger.
 * Messages are stored as cryptographically hashed nodes linked to parent hashes.
 * If any node is tampered with or deleted, ledger verification fails, prompting
 * physical capability revocation in seL4.
 */

#ifndef WIRE_MERKLE_DAG_HPP
#define WIRE_MERKLE_DAG_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <optional>

namespace wire::ledger {

using Hash256 = std::array<uint8_t, 32>;

/**
 * @struct DAGNode
 * @brief Represents a single immutable node in the Merkle-DAG ledger.
 */
struct DAGNode {
    uint64_t node_id;            /**< Monotonic node identifier index */
    uint64_t timestamp;          /**< Timestamp of node creation */
    Hash256 parent_hash;         /**< 256-bit hash of previous parent node */
    Hash256 ciphertext_hash;     /**< 256-bit hash of encrypted message payload */
    Hash256 node_hash;           /**< 256-bit cryptographic signature hash of this node */

    /**
     * @brief Computes node hash: BLAKE3_simulated(parent_hash || timestamp || ciphertext_hash).
     * @return Hash256 256-bit hash result.
     */
    [[nodiscard]] Hash256 compute_node_hash() const noexcept;
};

/**
 * @class MerkleDAGLedger
 * @brief Manages the append-only Merkle-DAG message history in Log_PD.
 */
class MerkleDAGLedger {
public:
    MerkleDAGLedger() noexcept;

    /**
     * @brief Appends a new encrypted message payload to the Merkle-DAG.
     * @param ciphertext Encrypted message payload bytes.
     * @param len Payload length.
     * @param timestamp System timestamp.
     * @return DAGNode Appended node record.
     */
    DAGNode append_message(const uint8_t* ciphertext, size_t len, uint64_t timestamp) noexcept;

    /**
     * @brief Verifies the cryptographic integrity of the entire Merkle-DAG chain.
     * @return true if ledger integrity is 100% valid, false if tampering or hash mismatch is detected.
     */
    [[nodiscard]] bool verify_ledger_integrity() const noexcept;

    /**
     * @brief Retrieves total node count in the ledger.
     * @return size_t Node count.
     */
    [[nodiscard]] size_t size() const noexcept { return m_nodes.size(); }

    /**
     * @brief Retrieves node by index.
     * @param index Node index.
     * @return std::optional<DAGNode> Node if exists, nullopt if out of bounds.
     */
    [[nodiscard]] std::optional<DAGNode> get_node(size_t index) const noexcept;

    /**
     * @brief Simulates ledger tampering (for testing integrity verification).
     * @param index Node index to tamper.
     */
    void tamper_node_for_test(size_t index) noexcept;

private:
    std::vector<DAGNode> m_nodes;
    Hash256 m_latest_root_hash{};
};

} // namespace wire::ledger

#endif // WIRE_MERKLE_DAG_HPP
