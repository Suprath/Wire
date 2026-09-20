/**
 * @file test_utf8_special_chars.cpp
 * @brief Test suite verifying special character, emoji, and multi-lingual UTF-8 message support
 * @project Project Wire
 */

#include "../src/crypto/double_ratchet.hpp"
#include "../src/ledger/merkle_dag.hpp"
#include "../src/bridge/virtio_serial_bridge.hpp"
#include "../src/ui/terminal_ui.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>

void test_special_characters_and_emojis() {
    std::cout << "[TEST] Running Special Characters, Emojis, & UTF-8 Encoding Test..." << std::endl;

    wire::crypto::Key256 root_key{};
    root_key.fill(0x77);

    wire::crypto::DoubleRatchet alice(root_key, true);
    wire::crypto::DoubleRatchet bob(root_key, false);

    wire::ledger::MerkleDAGLedger ledger;
    wire::bridge::VirtioSerialTunnel bridge(root_key);
    wire::ui::TerminalUI ui;

    // Test cases covering emojis, multi-lingual scripts, special symbols, JSON, quotes, newlines, tabs
    std::vector<std::string> test_messages = {
        "Hello World! Standard ASCII string.",
        "Emojis: 🚀 🔒 ⚡ 🔥 🛡️ 💬 🌐",
        "Multilingual: こんにちは (Japanese) | नमस्ते (Hindi) | Привет (Russian) | مرحبا (Arabic)",
        "Special Symbols: !@#$%^&*()_+-=[]{}|;':\",./<>?`~\\",
        "Control Chars: Line 1\nLine 2\tTabbed\rEscaped \\n \\t \\\" \\'",
        "{\"json_payload\": {\"status\": \"SECURE\", \"code\": 200, \"tags\": [\"seL4\", \"P2P\"]}}"
    };

    for (size_t i = 0; i < test_messages.size(); ++i) {
        const auto& original_msg = test_messages[i];
        std::cout << "  - Testing message #" << (i + 1) << " (" << original_msg.size() << " bytes)..." << std::endl;

        // 1. Virtio-Serial Bridge Encrypted Tunnel
        auto serial_frame = bridge.encrypt_host_frame(original_msg);
        auto decrypted_serial = bridge.decrypt_guest_frame(serial_frame.data(), serial_frame.size());
        assert(decrypted_serial.has_value());
        assert(decrypted_serial.value() == original_msg);

        // 2. Double Ratchet Encryption & Decryption
        auto [header, ciphertext] = alice.encrypt(
            reinterpret_cast<const uint8_t*>(original_msg.data()), original_msg.size());

        auto decrypted_ratchet = bob.decrypt(header, ciphertext.data(), ciphertext.size());
        assert(decrypted_ratchet.has_value());

        std::string recovered(decrypted_ratchet->begin(), decrypted_ratchet->end());
        assert(recovered == original_msg);

        // 3. Merkle-DAG Ledger Appending & Integrity
        auto node = ledger.append_message(ciphertext.data(), ciphertext.size(), 1700000000 + i);
        (void)node;
        assert(ledger.verify_ledger_integrity() == true);

        // Render UI
        ui.render_chat_message("ALICE", recovered);
    }

    std::cout << "  [PASS] All UTF-8 special characters, emojis, and control symbols passed cleanly without errors!" << std::endl;
    std::cout << "[SUCCESS] Special character encoding test passed!\n" << std::endl;
}

int main() {
    test_special_characters_and_emojis();
    return 0;
}
