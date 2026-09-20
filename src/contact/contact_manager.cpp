/**
 * @file contact_manager.cpp
 * @brief Implementation of Multi-Peer Contact Manager & Custom Alias Store
 * @project Project Wire
 */

#include "contact_manager.hpp"
#include <cstdio>
#include <algorithm>
#include <cctype>

namespace wire::contact {

std::string ContactManager::pubkey_to_hex(const crypto::Key256& key) noexcept {
    char buf[65];
    for (size_t i = 0; i < 32; ++i) {
        std::snprintf(buf + (i * 2), 3, "%02x", key[i]);
    }
    return std::string(buf, 64);
}

static std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

std::string ContactManager::add_contact(const std::string& custom_alias, const genesis::GenesisPayload& genesis_payload) noexcept {
    std::string contact_id = pubkey_to_hex(genesis_payload.peer_identity_pubkey);

    auto contact = std::make_unique<PeerContact>(contact_id, custom_alias, genesis_payload);
    m_alias_to_id[to_lower(custom_alias)] = contact_id;
    m_contacts[contact_id] = std::move(contact);

    return contact_id;
}

std::optional<std::string> ContactManager::add_contact_from_armored_file(const std::string& custom_alias, const std::string& armored_text) noexcept {
    auto parsed = genesis::GenesisManager::import_armored_key_file(armored_text);
    if (!parsed.has_value()) {
        return std::nullopt;
    }
    return add_contact(custom_alias, parsed.value());
}

PeerContact* ContactManager::find_contact_by_alias(const std::string& alias) noexcept {
    auto it = m_alias_to_id.find(to_lower(alias));
    if (it == m_alias_to_id.end()) {
        return nullptr;
    }
    return find_contact_by_id(it->second);
}

PeerContact* ContactManager::find_contact_by_id(const std::string& contact_id) noexcept {
    auto it = m_contacts.find(contact_id);
    if (it == m_contacts.end()) {
        return nullptr;
    }
    return it->second.get();
}

std::vector<std::pair<std::string, std::string>> ContactManager::get_contact_list() const noexcept {
    std::vector<std::pair<std::string, std::string>> list;
    list.reserve(m_contacts.size());

    for (const auto& [id, contact] : m_contacts) {
        std::string state_str;
        switch (contact->discovery->current_state()) {
            case network::PeerState::IDLE:      state_str = "IDLE"; break;
            case network::PeerState::SEARCHING: state_str = "SEARCHING"; break;
            case network::PeerState::CONNECTED: state_str = "CONNECTED"; break;
            case network::PeerState::OFFLINE:   state_str = "OFFLINE"; break;
        }
        list.emplace_back(contact->custom_alias, state_str);
    }
    return list;
}

} // namespace wire::contact
