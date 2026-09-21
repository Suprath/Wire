/**
 * @file contact_manager.hpp
 * @brief Multi-Peer Contact Vault & Concurrent Session Store for Project Wire
 * @project Project Wire
 * 
 * @details
 * Manages multiple peer contacts, local custom display aliases, and isolated session states.
 * 
 * Identity Rationale:
 * - Peers are cryptographically identified by their 256-bit Identity Public Key Hash.
 * - Custom display aliases (e.g. "Alice", "Bob (Work)") are assigned locally by the user.
 * - Local aliases are stored exclusively inside the local encrypted vault; they are NEVER
 *   broadcast over the network or shared on any registry server.
 * 
 * Multi-Chat Architecture:
 * - Each peer contact maintains an independent DoubleRatchet crypto session, MerkleDAGLedger,
 *   Kalman clock estimator, and On-Demand PredictiveDiscoveryEngine.
 */

#ifndef WIRE_CONTACT_MANAGER_HPP
#define WIRE_CONTACT_MANAGER_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include "../genesis/genesis_manager.hpp"
#include "../crypto/double_ratchet.hpp"
#include "../network/discovery_engine.hpp"
#include "../ledger/merkle_dag.hpp"

namespace wire::contact {

/**
 * @struct PeerContact
 * @brief Encapsulates the complete isolated session state for a single peer contact.
 */
struct PeerContact {
    std::string contact_id;                 /**< Hexadecimal string of 256-bit Identity Public Key */
    std::string custom_alias;               /**< User-assigned local display alias (e.g. "Bob") */
    genesis::GenesisPayload genesis_data;   /**< Genesis entanglement payload (seed, pubkey, T0, prefix) */
    
    std::unique_ptr<crypto::DoubleRatchet> ratchet;          /**< Signal-style Double Ratchet crypto session */
    std::unique_ptr<network::PredictiveDiscoveryEngine> discovery; /**< Independent On-Demand Discovery Engine */
    std::unique_ptr<ledger::MerkleDAGLedger> ledger;         /**< Independent Merkle-DAG message ledger */

    PeerContact(const std::string& id, const std::string& alias, const genesis::GenesisPayload& payload)
        : contact_id(id), custom_alias(alias), genesis_data(payload) {
        ratchet = std::make_unique<crypto::DoubleRatchet>(payload.master_seed, true);
        discovery = std::make_unique<network::PredictiveDiscoveryEngine>(payload.master_seed);
        ledger = std::make_unique<ledger::MerkleDAGLedger>();
    }
};

/**
 * @class ContactManager
 * @brief Manages multi-peer contact storage, local display aliases, and active chat threads.
 */
class ContactManager {
public:
    ContactManager() = default;

    /**
     * @brief Adds a new contact by importing a Genesis QR payload or Armored Text Key File with a custom alias.
     * @param custom_alias Local display name assigned by the user (e.g. "Bob", "Alice").
     * @param genesis_payload Genesis payload structure.
     * @return std::string Hexadecimal Contact ID.
     */
    std::string add_contact(const std::string& custom_alias, const genesis::GenesisPayload& genesis_payload) noexcept;

    /**
     * @brief Adds a new contact from an Armored Text Key File string (`wire_genesis.key`).
     * @param custom_alias Local display name (e.g. "Charlie").
     * @param armored_text Key file content.
     * @return std::optional<std::string> Contact ID if successfully parsed, nullopt if invalid key file.
     */
    std::optional<std::string> add_contact_from_armored_file(const std::string& custom_alias, const std::string& armored_text) noexcept;

    /**
     * @brief Retrieves a contact by custom local display alias.
     * @param alias Local alias (case-insensitive).
     * @return PeerContact* Pointer to contact session, or nullptr if not found.
     */
    [[nodiscard]] PeerContact* find_contact_by_alias(const std::string& alias) noexcept;

    /**
     * @brief Retrieves a contact by hexadecimal Contact ID.
     * @param contact_id Hexadecimal public key string.
     * @return PeerContact* Pointer to contact session, or nullptr if not found.
     */
    [[nodiscard]] PeerContact* find_contact_by_id(const std::string& contact_id) noexcept;

    /**
     * @brief Retrieves list of all contacts (aliases and states).
     * @return std::vector<std::pair<std::string, std::string>> List of (Alias, StateString) tuples.
     */
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> get_contact_list() const noexcept;

    /**
     * @brief Retrieves total number of active contacts.
     * @return size_t Count.
     */
    [[nodiscard]] size_t contact_count() const noexcept { return m_contacts.size(); }

    /**
     * @brief Converts 256-bit public key to 64-character hexadecimal string.
     */
    [[nodiscard]] static std::string pubkey_to_hex(const crypto::Key256& key) noexcept;

private:
    std::unordered_map<std::string, std::unique_ptr<PeerContact>> m_contacts; // ID -> Contact mapping
    std::unordered_map<std::string, std::string> m_alias_to_id;               // Alias -> ID mapping
};

} // namespace wire::contact

#endif // WIRE_CONTACT_MANAGER_HPP
