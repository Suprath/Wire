/**
 * @file test_double_ratchet.cpp
 * @brief Unit tests for Signal-Style Double Ratchet Protocol Engine
 * @project Project Wire
 */

#include "../src/crypto/double_ratchet.hpp"
#include <iostream>
#include <cassert>
#include <string>

void test_double_ratchet_handshake_and_encryption() {
    std::cout << "[TEST] Running Signal-Style Double Ratchet Protocol Test..." << std::endl;

    wire::crypto::Key256 shared_root_key{};
    shared_root_key.fill(0x33);

    // Initialize Alice (initiator) and Bob (responder)
    wire::crypto::DoubleRatchet alice(shared_root_key, true);
    wire::crypto::DoubleRatchet bob(shared_root_key, false);

    std::string secret_msg1 = "Project Wire: Absolute Privacy via seL4 Microkernel";

    // 1. Alice encrypts message for Bob
    auto [header1, ciphertext1] = alice.encrypt(reinterpret_cast<const uint8_t*>(secret_msg1.data()), secret_msg1.size());
    std::cout << "  - Alice encrypted message (" << secret_msg1.size() << " bytes -> " << ciphertext1.size() << " ciphertext bytes)." << std::endl;

    // 2. Bob decrypts message from Alice
    auto decrypted1 = bob.decrypt(header1, ciphertext1.data(), ciphertext1.size());
    assert(decrypted1.has_value());

    std::string recovered_msg1(decrypted1.value().begin(), decrypted1.value().end());
    assert(recovered_msg1 == secret_msg1);
    std::cout << "  [PASS] Alice -> Bob message encryption & decryption verified: '" << recovered_msg1 << "'" << std::endl;

    // 3. Bob replies to Alice
    std::string secret_msg2 = "ACK: Mathematical Knock Confirmed. Ghost Bridge Active.";
    auto [header2, ciphertext2] = bob.encrypt(reinterpret_cast<const uint8_t*>(secret_msg2.data()), secret_msg2.size());

    // 4. Alice decrypts reply from Bob
    auto decrypted2 = alice.decrypt(header2, ciphertext2.data(), ciphertext2.size());
    assert(decrypted2.has_value());

    std::string recovered_msg2(decrypted2.value().begin(), decrypted2.value().end());
    assert(recovered_msg2 == secret_msg2);
    std::cout << "  [PASS] Bob -> Alice reply encryption & ratchet advancement verified." << std::endl;

    // 5. Multi-message sequential test (Alice sends 5 messages in a row to Bob)
    for (int i = 0; i < 5; ++i) {
        std::string msg = "Sequential Alice Message #" + std::to_string(i + 1);
        auto [hdr, cipher] = alice.encrypt(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
        auto dec = bob.decrypt(hdr, cipher.data(), cipher.size());
        assert(dec.has_value());
        std::string rec(dec.value().begin(), dec.value().end());
        assert(rec == msg);
    }
    std::cout << "  [PASS] Multi-message sequential ratchet advancement verified (5 messages)." << std::endl;

    std::cout << "[SUCCESS] Double Ratchet tests passed cleanly!\n" << std::endl;
}

int main() {
    test_double_ratchet_handshake_and_encryption();
    return 0;
}
