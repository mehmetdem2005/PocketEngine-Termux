// ============================================================
//  pocket/engine/core/thread_pool.cpp
// ============================================================
#include "core/thread_pool.h"

namespace pk {

ThreadPool& globalThreadPool() noexcept {
    static ThreadPool pool;
    return pool;
}

} // namespace pk
