// ============================================================
//  pocket/engine/core/alloc.h
//  Custom allocators: arena, pool, scratch.
//  Engine never calls malloc/new directly in hot paths.
// ============================================================
#pragma once

#include "core/types.h"
#include "core/log.h"

#include <cstdlib>
#include <cstring>
#include <new>
#include <bitset>

namespace pk::alloc {

// ---------------------------------------------------------
//  Stats - thread-safe counters
// ---------------------------------------------------------
struct Stats {
    std::atomic<u64> totalAllocations{0};
    std::atomic<u64> totalDeallocations{0};
    std::atomic<u64> bytesAllocated{0};
    std::atomic<u64> bytesDeallocated{0};
    std::atomic<u64> peakBytes{0};
};

Stats& stats() noexcept;

inline void trackAllocate(usize bytes) noexcept {
    auto& s = stats();
    s.totalAllocations.fetch_add(1, std::memory_order_relaxed);
    auto cur = s.bytesAllocated.fetch_add(bytes, std::memory_order_relaxed) + bytes;
    auto peak = s.peakBytes.load(std::memory_order_relaxed);
    while (cur > peak && !s.peakBytes.compare_exchange_weak(peak, cur, std::memory_order_relaxed)) {}
}
inline void trackDeallocate(usize bytes) noexcept {
    auto& s = stats();
    s.totalDeallocations.fetch_add(1, std::memory_order_relaxed);
    s.bytesDeallocated.fetch_add(bytes, std::memory_order_relaxed);
}

// ---------------------------------------------------------
//  Linear Arena
//  Best for per-frame transient allocations
// ---------------------------------------------------------
class LinearArena {
public:
    explicit LinearArena(usize sizeBytes)
        : m_capacity(sizeBytes)
        , m_buffer(static_cast<u8*>(std::malloc(sizeBytes)))
        , m_offset(0) {
        if (!m_buffer) throw std::bad_alloc();
        std::memset(m_buffer, 0, sizeBytes);
    }
    ~LinearArena() { std::free(m_buffer); }

    LinearArena(const LinearArena&)            = delete;
    LinearArena& operator=(const LinearArena&) = delete;
    LinearArena(LinearArena&& o) noexcept
        : m_capacity(o.m_capacity), m_buffer(o.m_buffer), m_offset(o.m_offset.load()) {
        o.m_buffer = nullptr; o.m_capacity = 0;
    }

    [[nodiscard]] void* allocate(usize size, usize align = 16) noexcept {
        std::lock_guard<std::mutex> lk(m_mutex);
        usize aligned = (m_offset.load(std::memory_order_relaxed) + align - 1) & ~(align - 1);
        if (aligned + size > m_capacity) {
            PK_LOG_ERROR("Arena", "LinearArena out of memory: need %zu, have %zu",
                         aligned + size, m_capacity - m_offset.load());
            return nullptr;
        }
        void* p = m_buffer + aligned;
        m_offset.store(aligned + size, std::memory_order_relaxed);
        trackAllocate(size);
        return p;
    }

    void reset() noexcept {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_offset.store(0, std::memory_order_relaxed);
    }

    usize used()     const noexcept { return m_offset.load(std::memory_order_relaxed); }
    usize capacity() const noexcept { return m_capacity; }
    usize free()     const noexcept { return m_capacity - used(); }

private:
    usize                m_capacity;
    u8*                  m_buffer;
    std::atomic<usize>   m_offset;
    std::mutex           m_mutex;
};

// ---------------------------------------------------------
//  Pool Allocator
//  Fixed-size block pool. O(1) alloc/free.
// ---------------------------------------------------------
class PoolAllocator {
public:
    PoolAllocator(usize blockSize, usize blockCount, usize alignment = 16)
        : m_blockSize(((blockSize + alignment - 1) / alignment) * alignment)
        , m_blockCount(blockCount)
        , m_capacity(m_blockSize * blockCount)
        , m_buffer(static_cast<u8*>(std::malloc(m_capacity)))
        , m_freeList(nullptr) {
        if (!m_buffer) throw std::bad_alloc();
        // Build free list
        for (usize i = 0; i < blockCount; ++i) {
            u8* block = m_buffer + i * m_blockSize;
            *reinterpret_cast<u8**>(block) = (i + 1 < blockCount)
                ? (m_buffer + (i + 1) * m_blockSize)
                : nullptr;
        }
        m_freeList = reinterpret_cast<void**>(m_buffer);
    }

    ~PoolAllocator() { std::free(m_buffer); }

    PoolAllocator(const PoolAllocator&)            = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    [[nodiscard]] void* allocate() noexcept {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (!m_freeList) return nullptr;
        void* p = m_freeList;
        m_freeList = *reinterpret_cast<void**>(p);
        trackAllocate(m_blockSize);
        return p;
    }

    void deallocate(void* p) noexcept {
        if (!p) return;
        std::lock_guard<std::mutex> lk(m_mutex);
        *reinterpret_cast<void**>(p) = m_freeList;
        m_freeList = reinterpret_cast<void**>(p);
        trackDeallocate(m_blockSize);
    }

    usize blockSize() const noexcept { return m_blockSize; }
    usize capacity()  const noexcept { return m_capacity; }

private:
    usize       m_blockSize;
    usize       m_blockCount;
    usize       m_capacity;
    u8*         m_buffer;
    void**      m_freeList;
    std::mutex  m_mutex;
};

// ---------------------------------------------------------
//  Global allocator wrappers (new/delete overloads)
// ---------------------------------------------------------
void* alignedAlloc(usize size, usize align) noexcept;
void  alignedFree(void* p) noexcept;

// STL-compatible allocator backed by malloc (with tracking)
template <typename T>
struct TrackedAllocator {
    using value_type = T;
    TrackedAllocator() noexcept = default;
    template <typename U> TrackedAllocator(const TrackedAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(usize n) noexcept {
        usize bytes = n * sizeof(T);
        void* p = std::malloc(bytes);
        if (p) trackAllocate(bytes);
        return static_cast<T*>(p);
    }
    void deallocate(T* p, usize n) noexcept {
        if (!p) return;
        trackDeallocate(n * sizeof(T));
        std::free(p);
    }
    bool operator==(const TrackedAllocator&) const noexcept { return true; }
    bool operator!=(const TrackedAllocator&) const noexcept { return false; }
};

// Engine-wide frame arena (reset every frame)
LinearArena& frameArena() noexcept;

} // namespace pk::alloc

// ---------------------------------------------------------
//  Global new/delete tracking (debug builds)
// ---------------------------------------------------------
#ifdef POCKET_TRACK_GLOBAL_ALLOC
void* operator new(std::size_t n);
void  operator delete(void* p) noexcept;
void  operator delete(void* p, std::size_t n) noexcept;
#endif
