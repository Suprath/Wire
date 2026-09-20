/**
 * @file test_capability_severing.cpp
 * @brief Unit tests for seL4 Capability Severing Airlock Controller
 * @project Project Wire
 */

#include "../src/pd/capability_severing.hpp"
#include <iostream>
#include <cassert>

void test_capability_severing_controller() {
    std::cout << "[TEST] Running seL4 Capability Severing Airlock Test..." << std::endl;

    wire::pd::CapabilitySeveringController airlock(3); // Threshold = 3 faults

    assert(airlock.state() == wire::pd::AirLockState::CONNECTED);
    assert(airlock.fault_count() == 0);
    std::cout << "  [PASS] Initial state verified: CONNECTED (Grants intact)." << std::endl;

    // Record Fault 1
    auto state1 = airlock.record_fault();
    assert(state1 == wire::pd::AirLockState::FAULTED);
    assert(airlock.fault_count() == 1);

    // Record Fault 2
    auto state2 = airlock.record_fault();
    assert(state2 == wire::pd::AirLockState::FAULTED);
    assert(airlock.fault_count() == 2);

    // Record Fault 3 -> Threshold reached -> SEVERED
    auto state3 = airlock.record_fault();
    assert(state3 == wire::pd::AirLockState::SEVERED);
    assert(airlock.state() == wire::pd::AirLockState::SEVERED);
    std::cout << "  [PASS] Intrusion threshold reached. Physical capability revocation triggered: SEVERED." << std::endl;

    std::cout << "[SUCCESS] Capability severing airlock tests passed cleanly!\n" << std::endl;
}

int main() {
    test_capability_severing_controller();
    return 0;
}
