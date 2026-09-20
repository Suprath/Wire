/**
 * @file test_discovery_engine.cpp
 * @brief Unit tests for On-Demand Discovery Engine & Kalman Filter Clock Sync
 * @project Project Wire
 */

#include "../src/network/discovery_engine.hpp"
#include <iostream>
#include <cassert>

void test_on_demand_discovery_lifecycle() {
    std::cout << "[TEST] Running On-Demand Discovery State Machine & Timeout Test..." << std::endl;

    wire::crypto::ChaCha20PRNG::Seed256 seed{};
    seed.fill(0xAA);

    wire::network::PredictiveDiscoveryEngine discovery(seed, 900); // 15-min timeout

    // Initial state: IDLE
    assert(discovery.current_state() == wire::network::PeerState::IDLE);
    std::cout << "  [PASS] Initial state verified: IDLE." << std::endl;

    // Start discovery
    discovery.start_on_demand_search();
    assert(discovery.current_state() == wire::network::PeerState::SEARCHING);
    assert(discovery.current_tier() == wire::network::SearchTier::TIER_1_GENESIS_SUBNETS);
    std::cout << "  [PASS] Search started. Active tier: Tier 1 (Genesis Subnets)." << std::endl;

    // Tick at 120s -> Tier 2 Escalation (Global BGP Scan)
    discovery.tick(120);
    assert(discovery.current_tier() == wire::network::SearchTier::TIER_2_GLOBAL_BGP_SCAN);
    std::cout << "  [PASS] Search escalated to Tier 2 (Global BGP Scan at 666 PPS)." << std::endl;

    // Tick at 700s -> Tier 3 Escalation (NAT Hole Punching)
    discovery.tick(700);
    assert(discovery.current_tier() == wire::network::SearchTier::TIER_3_NAT_HOLE_PUNCH);
    std::cout << "  [PASS] Search escalated to Tier 3 (Bi-Directional NAT Hole Punching)." << std::endl;

    // Tick at 950s -> Bounded Search Timeout -> OFFLINE
    discovery.tick(950);
    assert(discovery.current_state() == wire::network::PeerState::OFFLINE);
    std::cout << "  [PASS] 15-minute search timeout boundary verified. Peer marked OFFLINE." << std::endl;

    // Test Kalman Filter clock drift estimation
    discovery.start_on_demand_search();
    discovery.update_kalman_clock_drift(10.0, 1000.0, 995.0); // 10ms RTT, measured 0ms bias
    assert(discovery.current_state() == wire::network::PeerState::CONNECTED);
    std::cout << "  [PASS] Kalman clock update verified. Peer state transitioned to CONNECTED." << std::endl;

    std::cout << "[SUCCESS] Discovery engine tests passed cleanly!\n" << std::endl;
}

int main() {
    test_on_demand_discovery_lifecycle();
    return 0;
}
