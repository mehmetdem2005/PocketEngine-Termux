// ============================================================
//  pocket/engine/core/alloc.cpp
// ============================================================
#include "core/alloc.h"

#include <cstdlib>
#include <malloc.h>

namespace pk::alloc {

Stats& stats() noexcept {
    static Stats s;
    return s;
}

void* alignedAlloc(usize size, usize align) noexcept {
    void* p = nullptr;
#if defined(_WIN32)
    p = _aligned_malloc(size, align);
#else
    if (posix_memalign(&p, align > sizeof(void*) ? align : sizeof(void*), size) != 0)
        p = nullptr;
#endif
    if (p) trackAllocate(size);
    return p;
}

void alignedFree(void* p) noexcept {
    if (!p) return;
    std::free(p);
}

LinearArena& frameArena() noexcept {
    static LinearArena arena(16 * 1024 * 1024);  // 16MB frame arena
    return arena;
}

} // namespace pk::alloc

#ifdef POCKET_TRACK_GLOBAL_ALLOC
void* operator new(std::size_t n) {
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    pk::alloc::trackAllocate(n);
    return p;
}
void operator delete(void* p) noexcept {
    if (!p) return;
    pk::alloc::trackDeallocate(0);
    std::free(p);
}
void operator delete(void* p, std::size_t n) noexcept {
    if (!p) return;
    pk::alloc::trackDeallocate(n);
    std::free(p);
}
#endif
