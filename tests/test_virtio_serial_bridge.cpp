/**
 * @file test_virtio_serial_bridge.cpp
 * @brief Unit tests for Encrypted Host-Guest Virtio-Serial Tunnel
 * @project Project Wire
 */

#include "../src/bridge/virtio_serial_bridge.hpp"
#include <iostream>
#include <cassert>
#include <string>

void test_virtio_serial_tunnel() {
    std::cout << "[TEST] Running Encrypted Host-Guest Virtio-Serial Tunnel Test..." << std::endl;

    wire::bridge::SessionKey256 session_key{};
    session_key.fill(0x44);

    wire::bridge::VirtioSerialTunnel tunnel(session_key);

    std::string host_command = "CMD_START_ON_DEMAND_SEARCH peer_id=Bob";

    // 1. Host encrypts frame to seL4
    auto encrypted_frame = tunnel.encrypt_host_frame(host_command);
    assert(encrypted_frame.size() == host_command.size() + 16);
    std::cout << "  - Host encrypted serial command (" << host_command.size() << " chars -> " << encrypted_frame.size() << " frame bytes)." << std::endl;

    // 2. seL4 Monitor_PD decrypts frame from host
    auto decrypted = tunnel.decrypt_guest_frame(encrypted_frame.data(), encrypted_frame.size());
    assert(decrypted.has_value());
    assert(decrypted.value() == host_command);

    std::cout << "  [PASS] Virtio-Serial frame encryption & decryption verified: '" << decrypted.value() << "'" << std::endl;

    std::cout << "[SUCCESS] Virtio-serial bridge tests passed cleanly!\n" << std::endl;
}

int main() {
    test_virtio_serial_tunnel();
    return 0;
}
