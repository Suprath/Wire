/**
 * @file discovery_engine.hpp
 * @brief On-Demand Predictive Discovery Engine & Kalman Filter Clock Drift Estimator
 * @project Project Wire
 * 
 * @details
 * Manages the on-demand predictive discovery state machine, Tier 1..3 search transitions,
 * 15-minute search timeouts, and Kalman Filter clock drift tracking.
 */

#ifndef WIRE_DISCOVERY_ENGINE_HPP
#define WIRE_DISCOVERY_ENGINE_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <chrono>
#include "../crypto/chacha20_prng.hpp"

namespace wire::network {

// Peer Discovery State
enum class PeerState {
    IDLE,        /**< Idle state. Discovery inactive. */
    SEARCHING,   /**< Active searching across Tier 1..3 subnets. */
    CONNECTED,   /**< Bi-directional connection established. */
    OFFLINE      /**< Bounded search window expired without response. Peer marked offline. */
};

// Search Tier Strategy
enum class SearchTier {
    TIER_1_GENESIS_SUBNETS, /**< Probe Genesis QR/File IPv6 subnets */
    TIER_2_GLOBAL_BGP_SCAN, /**< Scan ~200k global BGP prefixes at 666 PPS */
    TIER_3_NAT_HOLE_PUNCH   /**< Outbound NAT hole-punch pings for institutional Wi-Fi */
};

/**
 * @struct KalmanClockState
 * @brief Represents the 2D Kalman Filter state for crystal clock drift estimation.
 */
struct KalmanClockState {
    double clock_bias_ms{0.0};  /**< Estimated clock bias delta_t in milliseconds */
    double drift_rate_ms_per_hr{0.0}; /**< Estimated drift rate dot_t in ms/hour */
    double covariance_p00{1.0}; /**< Variance estimate for bias */
    double covariance_p11{0.1}; /**< Variance estimate for drift rate */
};

/**
 * @class PredictiveDiscoveryEngine
 * @brief Manages on-demand discovery lifecycle and Kalman clock drift tracking.
 */
class PredictiveDiscoveryEngine {
public:
    /**
     * @brief Constructs Discovery Engine with peer Master Seed.
     * @param master_seed 256-bit shared master seed from Genesis exchange.
     * @param search_timeout_sec Maximum search duration in seconds (default 900s = 15 mins).
     */
    explicit PredictiveDiscoveryEngine(
        const crypto::ChaCha20PRNG::Seed256& master_seed,
        uint32_t search_timeout_sec = 900) noexcept;

    /**
     * @brief Triggers on-demand discovery when user opens chat or sends message.
     */
    void start_on_demand_search() noexcept;

    /**
     * @brief Cancels active discovery and returns state to IDLE.
     */
    void stop_search() noexcept;

    /**
     * @brief Advances the discovery search tick, evaluating search timeout boundaries.
     * @param elapsed_sec Seconds elapsed since search initiation.
     * @return PeerState Current state of peer discovery.
     */
    PeerState tick(uint32_t elapsed_sec) noexcept;

    /**
     * @brief Updates Kalman filter clock drift model using a measured RTT timestamp delta.
     * @param rtt_ms Measured Round-Trip-Time in milliseconds.
     * @param peer_timestamp_ms Peer's reported epoch timestamp in milliseconds.
     * @param local_timestamp_ms Local system timestamp in milliseconds.
     */
    void update_kalman_clock_drift(double rtt_ms, double peer_timestamp_ms, double local_timestamp_ms) noexcept;

    /**
     * @brief Retrieves current state of the discovery state machine.
     * @return PeerState Current state enum.
     */
    [[nodiscard]] PeerState current_state() const noexcept { return m_state; }

    /**
     * @brief Retrieves current active search tier.
     * @return SearchTier Current tier enum.
     */
    [[nodiscard]] SearchTier current_tier() const noexcept { return m_current_tier; }

    /**
     * @brief Retrieves current Kalman clock drift state.
     * @return KalmanClockState Struct containing bias and drift rate estimates.
     */
    [[nodiscard]] KalmanClockState kalman_state() const noexcept { return m_kalman; }

private:
    crypto::ChaCha20PRNG m_prng;
    PeerState m_state{PeerState::IDLE};
    SearchTier m_current_tier{SearchTier::TIER_1_GENESIS_SUBNETS};
    uint32_t m_search_timeout_sec;
    KalmanClockState m_kalman{};
};

} // namespace wire::network

#endif // WIRE_DISCOVERY_ENGINE_HPP
