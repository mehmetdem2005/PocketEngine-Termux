// ============================================================
//  pocket/engine/core/event.cpp
// ============================================================
#include "core/event.h"

namespace pk::event {

Bus& bus() noexcept {
    static Bus b;
    return b;
}

} // namespace pk::event
