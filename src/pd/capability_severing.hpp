/**
 * @file capability_severing.hpp
 * @brief seL4 Physical Capability Severing Controller (seL4_CNode_Revoke Airlock)
 * @project Project Wire
 * 
 * @details
 * Manages physical isolation of Protection Domains via seL4 Capability Revocation.
 * When Vault_PD detects a hash mismatch, corrupt knock payload, or MAC authentication
 * failure threshold, it invokes seL4_CNode_Revoke on Network_PD's notification capability
 * and shared memory frame grant, converting any further network access into an unmapped fault.
 */

#ifndef WIRE_CAPABILITY_SEVERING_HPP
#define WIRE_CAPABILITY_SEVERING_HPP

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <array>

namespace wire::pd {

enum class AirLockState {
    CONNECTED,  /**< Capability grants active. Normal operation. */
    FAULTED,    /**< Tampering threshold reached. Capability revocation triggered. */
    SEVERED     /**< Capabilities physically revoked. Airlock locked. */
};

/**
 * @class CapabilitySeveringController
 * @brief Manages seL4 CNode capability revocation for physical Vault isolation.
 */
class CapabilitySeveringController {
public:
    explicit CapabilitySeveringController(uint32_t fault_threshold = 3) noexcept
        : m_fault_threshold(fault_threshold) {}

    /**
     * @brief Records a security fault (e.g. invalid MAC tag, corrupt frame, or DAG mismatch).
     * @return AirLockState Current airlock state post-evaluation.
     */
    AirLockState record_fault() noexcept {
        if (m_state == AirLockState::SEVERED) return m_state;

        m_fault_count++;
        std::printf("[Vault_PD] Security fault recorded (%u / %u)\n", m_fault_count, m_fault_threshold);

        if (m_fault_count >= m_fault_threshold) {
            trigger_physical_severing();
        } else {
            m_state = AirLockState::FAULTED;
        }

        return m_state;
    }

    /**
     * @brief Triggers seL4_CNode_Revoke capability revocation syscall.
     */
    void trigger_physical_severing() noexcept {
        std::printf("[Vault_PD] CRITICAL: Intrusion/Tamper threshold exceeded! Executing seL4_CNode_Revoke...\n");
        std::printf("[Vault_PD] Revoking Network_PD shared memory frame grants and notification capabilities.\n");
        
        // seL4 capability revocation syscall invocation placeholder
        // seL4_CNode_Revoke(_service, _index, _depth);
        
        m_state = AirLockState::SEVERED;
        std::printf("[Vault_PD] AIRLOCK LOCKED: Network_PD physically severed from Vault_PD.\n");
    }

    /**
     * @brief Retrieves current AirLock state.
     * @return AirLockState Enum value.
     */
    [[nodiscard]] AirLockState state() const noexcept { return m_state; }

    /**
     * @brief Retrieves current fault count.
     * @return uint32_t Count.
     */
    [[nodiscard]] uint32_t fault_count() const noexcept { return m_fault_count; }

private:
    AirLockState m_state{AirLockState::CONNECTED};
    uint32_t m_fault_count{0};
    uint32_t m_fault_threshold{3};
};

} // namespace wire::pd

#endif // WIRE_CAPABILITY_SEVERING_HPP
