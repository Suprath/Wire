/**
 * @file main_tui.cpp
 * @brief Standalone Terminal UI Client Application for Project Wire
 * @project Project Wire
 */

#include "terminal_ui.hpp"
#include "../genesis/genesis_manager.hpp"
#include "../contact/contact_manager.hpp"
#include "../update/update_manager.hpp"
#include <iostream>
#include <string>

int main() {
    wire::ui::TerminalUI ui;
    wire::contact::ContactManager contacts;

    // Initialize mock entanglement with Peer_B (Bob)
    wire::genesis::Key256 seed{}; seed.fill(0x77);
    wire::genesis::Key256 pubkey{}; pubkey.fill(0xBB);
    auto payload_bob = wire::genesis::GenesisManager::create_genesis(seed, pubkey, 1700000000, 0x20010db800000000ULL);

    std::string bob_id = contacts.add_contact("Bob", payload_bob);
    auto* bob = contacts.find_contact_by_alias("Bob");

    // Render Status Dashboard
    ui.render_status_header("Bob (Peer B)", bob->discovery->current_state(), bob->discovery->current_tier());

    std::cout << "[SYSTEM] Initialized seL4 Micro-VM QEMU hvf Session (aarch64).\n";
    std::cout << "[SYSTEM] Vault_PD: Master Seed loaded. Capabilities granted.\n";
    std::cout << "[SYSTEM] Data Vault Storage: " << wire::update::UpdateManager::get_user_data_directory() << "\n\n";

    // Simulate sending an encrypted message
    std::cout << "[USER ACTION] User initiates conversation with Bob...\n";
    bob->discovery->start_on_demand_search();

    ui.render_status_header("Bob (Peer B)", bob->discovery->current_state(), bob->discovery->current_tier());

    std::string msg1 = "Hello Bob! Testing Project Wire over seL4 Microkernel.";
    auto [hdr1, cipher1] = bob->ratchet->encrypt(reinterpret_cast<const uint8_t*>(msg1.data()), msg1.size());
    bob->ledger->append_message(cipher1.data(), cipher1.size(), 1700000010);

    ui.render_chat_message("YOU (Vault_PD)", msg1);

    // Simulate receiving an encrypted reply
    std::string reply1 = "ACK: Mathematical Knock Confirmed on Port 6853. Ghost Bridge Connected! 🚀";
    bob->discovery->update_kalman_clock_drift(8.5, 1700000015.0, 1700000010.0);

    ui.render_status_header("Bob (Peer B)", bob->discovery->current_state(), bob->discovery->current_tier());
    ui.render_chat_message("BOB (Peer_B)", reply1);

    std::cout << "\n=======================================================================\n";
    std::cout << "  PROJECT WIRE RUNTIME READY: All PDs Operational & Verified.\n";
    std::cout << "=======================================================================\n";

    return 0;
}
