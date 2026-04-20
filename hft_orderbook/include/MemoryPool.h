#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cassert>
#include <new>
#include <vector>  // BUG FIX #1: Was missing — ArenaAllocator uses std::vector<std::byte>

/**
 * @brief Fixed-size object pool using a lock-free free list.
 *        Eliminates heap allocation latency in the hot path.
 *        Thread-safe for concurrent alloc/free.
 */
template<typename T, size_t PoolSize>
class ObjectPool {
    struct alignas(alignof(T)) Block {
        std::byte storage[sizeof(T)];
        std::atomic<Block*> next{nullptr};
    };

    alignas(64) std::array<Block, PoolSize> pool_;
    alignas(64) std::atomic<Block*> freeHead_{nullptr};
    alignas(64) std::atomic<size_t> allocated_{0};
    alignas(64) std::atomic<size_t> totalAllocs_{0};

public:
    ObjectPool() {
        for (size_t i = 0; i < PoolSize - 1; ++i)
            pool_[i].next.store(&pool_[i + 1], std::memory_order_relaxed);
        pool_[PoolSize - 1].next.store(nullptr, std::memory_order_relaxed);
        freeHead_.store(&pool_[0], std::memory_order_release);
    }

    template<typename... Args>
    T* allocate(Args&&... args) {
        Block* block = popFreeList();
        if (!block) return nullptr;
        ++allocated_;
        ++totalAllocs_;
        return new (block->storage) T(std::forward<Args>(args)...);
    }

    void deallocate(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        Block* block = reinterpret_cast<Block*>(reinterpret_cast<std::byte*>(ptr));
        pushFreeList(block);
        --allocated_;
    }

    size_t available()    const noexcept { return PoolSize - allocated_.load(std::memory_order_relaxed); }
    size_t totalAllocs()  const noexcept { return totalAllocs_.load(); }

private:
    Block* popFreeList() noexcept {
        Block* head = freeHead_.load(std::memory_order_acquire);
        while (head) {
            Block* next = head->next.load(std::memory_order_relaxed);
            if (freeHead_.compare_exchange_weak(head, next,
                    std::memory_order_release, std::memory_order_acquire))
                return head;
        }
        return nullptr;
    }

    void pushFreeList(Block* block) noexcept {
        Block* head = freeHead_.load(std::memory_order_relaxed);
        do {
            block->next.store(head, std::memory_order_relaxed);
        } while (!freeHead_.compare_exchange_weak(head, block,
                     std::memory_order_release, std::memory_order_relaxed));
    }
};

/**
 * @brief Arena allocator for bulk allocations with O(1) reset.
 */
class ArenaAllocator {
    std::vector<std::byte> buf_;
    size_t offset_{0};
public:
    explicit ArenaAllocator(size_t sizeBytes) : buf_(sizeBytes) {}

    void* allocate(size_t bytes, size_t alignment = 8) {
        size_t aligned = (offset_ + alignment - 1) & ~(alignment - 1);
        if (aligned + bytes > buf_.size()) return nullptr;
        void* ptr = buf_.data() + aligned;
        offset_ = aligned + bytes;
        return ptr;
    }

    void reset()     noexcept { offset_ = 0; }
    size_t used()    const noexcept { return offset_; }
    size_t capacity() const noexcept { return buf_.size(); }
};
