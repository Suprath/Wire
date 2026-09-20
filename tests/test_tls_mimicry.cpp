/**
 * @file test_tls_mimicry.cpp
 * @brief Unit tests for HTTPS / Port-443 TLS 1.3 Record Layer Obfuscation Engine
 * @project Project Wire
 */

#include "../src/network/tls_mimicry.hpp"
#include <iostream>
#include <cassert>

void test_tls_13_record_wrapping() {
    std::cout << "[TEST] Running HTTPS / Port-443 TLS 1.3 Mimicry Test..." << std::endl;

    std::vector<uint8_t> raw_payload = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };

    // 1. Wrap raw payload in TLS 1.3 Application Data frame (0x17)
    auto tls_frame = wire::network::TLSMimicryEngine::wrap_tls_application_data(raw_payload.data(), raw_payload.size());

    // Verify TLS record header (5 bytes header + payload length)
    assert(tls_frame.size() == 5 + raw_payload.size());
    assert(tls_frame[0] == wire::network::TLS_RECORD_APPLICATION); // 0x17
    assert(tls_frame[1] == 0x03 && tls_frame[2] == 0x03);          // TLS 1.2 legacy version string (0x0303)

    std::cout << "  - Wrapped " << raw_payload.size() << " bytes into TLS 1.3 frame (" << tls_frame.size() << " total bytes)." << std::endl;

    // 2. Unwrap TLS record envelope
    auto unwrapped = wire::network::TLSMimicryEngine::unwrap_tls_record(tls_frame.data(), tls_frame.size());
    assert(unwrapped.has_value());
    assert(unwrapped.value() == raw_payload);

    std::cout << "  [PASS] TLS 1.3 frame encapsulation & decapsulation verified." << std::endl;

    // 3. Test ClientHello generation
    auto client_hello = wire::network::TLSMimicryEngine::generate_client_hello("cdn.cloudflare.net");
    assert(client_hello[0] == wire::network::TLS_RECORD_HANDSHAKE); // 0x16 Handshake
    std::cout << "  [PASS] Simulated TLS ClientHello synthesis verified." << std::endl;

    std::cout << "[SUCCESS] TLS mimicry tests passed cleanly!\n" << std::endl;
}

int main() {
    test_tls_13_record_wrapping();
    return 0;
}
