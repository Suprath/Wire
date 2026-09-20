/**
 * @file sddf_net.hpp
 * @brief LionsOS sDDF (System Device Driver Framework) Zero-Copy Network Ring Buffer
 * @project Project Wire
 * 
 * @details
 * Defines the shared memory ring buffer interface between Network_PD and Vault_PD.
 * Implements lock-free single-producer single-consumer (SPSC) ring buffer descriptors
 * for zero-copy memory transfers over seL4 shared memory frames (net_vault_ring).
 */

#ifndef WIRE_SDDF_NET_HPP
#define WIRE_SDDF_NET_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <atomic>

namespace wire::sddf {

/**
 * @struct BufferDescriptor
 * @brief Represents a zero-copy memory frame descriptor in the sDDF shared ring buffer.
 */
struct BufferDescriptor {
    uint64_t io_or_offset; /**< Offset in shared memory region */
    uint32_t len;          /**< Length of packet payload in bytes */
    uint32_t flags;        /**< Status flags (e.g., SDDF_NET_FLAG_ENCRYPTED, SDDF_NET_FLAG_KNOCK) */
};

// Flags for BufferDescriptor
constexpr uint32_t SDDF_FLAG_VALID     = (1U << 0);
constexpr uint32_t SDDF_FLAG_ENCRYPTED = (1U << 1);
constexpr uint32_t SDDF_FLAG_KNOCK     = (1U << 2);

/**
 * @class NetRingBuffer
 * @brief Lock-free Single-Producer Single-Consumer (SPSC) sDDF Ring Buffer.
 */
template <size_t Capacity = 64>
class NetRingBuffer {
public:
    NetRingBuffer() : m_head(0), m_tail(0) {}

    /**
     * @brief Enqueues a packet descriptor into the ring buffer (Producer thread/PD).
     * @param desc Descriptor containing packet offset and length.
     * @return true if successfully enqueued, false if ring buffer is full.
     */
    bool enqueue(const BufferDescriptor& desc) noexcept {
        size_t current_tail = m_tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Capacity;

        if (next_tail == m_head.load(std::memory_order_acquire)) {
            return false; // Ring buffer full
        }

        m_descriptors[current_tail] = desc;
        m_tail.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Dequeues a packet descriptor from the ring buffer (Consumer thread/PD).
     * @param desc Output parameter to receive the packet descriptor.
     * @return true if successfully dequeued, false if ring buffer is empty.
     */
    bool dequeue(BufferDescriptor& desc) noexcept {
        size_t current_head = m_head.load(std::memory_order_relaxed);

        if (current_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Ring buffer empty
        }

        desc = m_descriptors[current_head];
        m_head.store((current_head + 1) % Capacity, std::memory_order_release);
        return true;
    }

    /**
     * @brief Checks if the ring buffer is empty.
     * @return true if empty.
     */
    [[nodiscard]] bool empty() const noexcept {
        return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
    }

private:
    std::array<BufferDescriptor, Capacity> m_descriptors{};
    alignas(64) std::atomic<size_t> m_head;
    alignas(64) std::atomic<size_t> m_tail;
};

} // namespace wire::sddf

#endif // WIRE_SDDF_NET_HPP
