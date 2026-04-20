#pragma once
#include <atomic>
#include <array>
#include <optional>
#include <cstddef>
#include <cassert>

/**
 * @brief Lock-free Single-Producer Single-Consumer ring buffer.
 *        Cache-line padded to eliminate false sharing.
 *        Capacity must be power of 2.
 *
 * BUG FIX: Original stored (t+1) & Mask into tail_, which collapsed the
 * index to the range [0, Capacity-1]. This caused the "full" check
 *   next == head_
 * to trigger when next wrapped to 0 and head_ was also 0 (i.e. after
 * exactly Capacity pushes without a pop), but it also made size() return
 * wrong values because the raw distance calculation assumed monotonically
 * growing indices. The standard SPSC pattern stores raw (never-masked)
 * indices and only applies the mask when indexing into buf_. Fixed below.
 */
template<typename T, size_t Capacity>
class alignas(64) SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    static constexpr size_t Mask = Capacity - 1;

    struct alignas(64) PaddedAtomic {
        std::atomic<size_t> val{0};
        char pad[64 - sizeof(std::atomic<size_t>)]{};
    };

    alignas(64) std::array<T, Capacity> buf_;
    PaddedAtomic head_;   // written by consumer (monotonically increasing)
    PaddedAtomic tail_;   // written by producer (monotonically increasing)

public:
    SPSCQueue() = default;
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Producer side
    bool push(const T& item) noexcept {
        const size_t t    = tail_.val.load(std::memory_order_relaxed);
        const size_t next = t + 1;
        // Full check: the queue is full if the slot next would occupy is still
        // held by the consumer (i.e. next & Mask == head_ & Mask, and we've
        // lapped). Simplified: if (next - head) == Capacity → full.
        if (next - head_.val.load(std::memory_order_acquire) > Mask)
            return false;
        buf_[t & Mask] = item;
        tail_.val.store(next, std::memory_order_release);
        return true;
    }

    bool push(T&& item) noexcept {
        const size_t t    = tail_.val.load(std::memory_order_relaxed);
        const size_t next = t + 1;
        if (next - head_.val.load(std::memory_order_acquire) > Mask)
            return false;
        buf_[t & Mask] = std::move(item);
        tail_.val.store(next, std::memory_order_release);
        return true;
    }

    // Consumer side
    std::optional<T> pop() noexcept {
        const size_t h = head_.val.load(std::memory_order_relaxed);
        if (h == tail_.val.load(std::memory_order_acquire))
            return std::nullopt;
        T item = std::move(buf_[h & Mask]);
        head_.val.store(h + 1, std::memory_order_release);
        return item;
    }

    bool empty() const noexcept {
        return head_.val.load(std::memory_order_acquire) ==
               tail_.val.load(std::memory_order_acquire);
    }

    size_t size() const noexcept {
        const size_t t = tail_.val.load(std::memory_order_acquire);
        const size_t h = head_.val.load(std::memory_order_acquire);
        return t - h;  // correct because indices are monotonic, not masked
    }
};

/**
 * @brief Lock-free MPMC queue using atomic CAS.
 */
template<typename T, size_t Capacity>
class MPMCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    struct Slot {
        alignas(64) std::atomic<size_t> seq;
        T data;
    };

    alignas(64) std::array<Slot, Capacity> slots_;
    alignas(64) std::atomic<size_t> enqueuePos_{0};
    alignas(64) std::atomic<size_t> dequeuePos_{0};

    static constexpr size_t Mask = Capacity - 1;

public:
    MPMCQueue() {
        for (size_t i = 0; i < Capacity; ++i)
            slots_[i].seq.store(i, std::memory_order_relaxed);
    }

    bool push(T&& item) noexcept {
        size_t pos = enqueuePos_.load(std::memory_order_relaxed);
        while (true) {
            Slot& slot = slots_[pos & Mask];
            size_t seq = slot.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            if (diff == 0) {
                if (enqueuePos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    slot.data = std::move(item);
                    slot.seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = enqueuePos_.load(std::memory_order_relaxed);
            }
        }
    }

    bool pop(T& item) noexcept {
        size_t pos = dequeuePos_.load(std::memory_order_relaxed);
        while (true) {
            Slot& slot = slots_[pos & Mask];
            size_t seq = slot.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            if (diff == 0) {
                if (dequeuePos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    item = std::move(slot.data);
                    slot.seq.store(pos + Capacity, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = dequeuePos_.load(std::memory_order_relaxed);
            }
        }
    }
};
