/**
 * @file discovery_engine.cpp
 * @brief Implementation of On-Demand Discovery Engine & Kalman Clock Drift Estimator
 * @project Project Wire
 */

#include "discovery_engine.hpp"
#include <cmath>
#include <algorithm>

namespace wire::network {

PredictiveDiscoveryEngine::PredictiveDiscoveryEngine(
    const crypto::ChaCha20PRNG::Seed256& master_seed,
    uint32_t search_timeout_sec) noexcept
    : m_prng(master_seed), m_search_timeout_sec(search_timeout_sec) {}

void PredictiveDiscoveryEngine::start_on_demand_search() noexcept {
    m_state = PeerState::SEARCHING;
    m_current_tier = SearchTier::TIER_1_GENESIS_SUBNETS;
}

void PredictiveDiscoveryEngine::stop_search() noexcept {
    m_state = PeerState::IDLE;
}

PeerState PredictiveDiscoveryEngine::tick(uint32_t elapsed_sec) noexcept {
    if (m_state != PeerState::SEARCHING) {
        return m_state;
    }

    // Evaluate 15-minute search timeout boundary
    if (elapsed_sec >= m_search_timeout_sec) {
        m_state = PeerState::OFFLINE; // Bounded search expired
        return m_state;
    }

    // Tier escalation strategy:
    // 0..60s: Tier 1 (Genesis Subnets)
    // 60s..600s: Tier 2 (Global BGP Scanning)
    // 600s..900s: Tier 3 (Bi-Directional NAT Hole Punching)
    if (elapsed_sec < 60) {
        m_current_tier = SearchTier::TIER_1_GENESIS_SUBNETS;
    } else if (elapsed_sec < 600) {
        m_current_tier = SearchTier::TIER_2_GLOBAL_BGP_SCAN;
    } else {
        m_current_tier = SearchTier::TIER_3_NAT_HOLE_PUNCH;
    }

    return m_state;
}

void PredictiveDiscoveryEngine::update_kalman_clock_drift(double rtt_ms, double peer_timestamp_ms, double local_timestamp_ms) noexcept {
    // Measured offset: z_k = peer_timestamp - (local_timestamp + RTT/2)
    double one_way_delay = rtt_ms / 2.0;
    double measured_bias = peer_timestamp_ms - (local_timestamp_ms + one_way_delay);

    // Kalman measurement update
    constexpr double R_measurement_variance = 5.0; // 5ms RTT measurement noise
    double K_gain = m_kalman.covariance_p00 / (m_kalman.covariance_p00 + R_measurement_variance);

    // Update state vector
    m_kalman.clock_bias_ms += K_gain * (measured_bias - m_kalman.clock_bias_ms);
    m_kalman.covariance_p00 *= (1.0 - K_gain);

    // Transition to CONNECTED state upon successful timing handshake
    m_state = PeerState::CONNECTED;
}

} // namespace wire::network
