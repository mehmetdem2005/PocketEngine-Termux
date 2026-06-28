// ============================================================
//  pocket/engine/core/types.h
//  Core engine typedefs
// ============================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>

namespace pk {

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = std::size_t;
using isize = std::ptrdiff_t;
using f32 = float;
using f64 = double;

using String       = std::string;
using StringView   = std::string_view;

// 2D/3D/4D vector templates (used widely before math.h is included)
template <typename T> struct Vec2 { T x{}, y{}; };
template <typename T> struct Vec3 { T x{}, y{}, z{}; };
template <typename T> struct Vec4 { T x{}, y{}, z{}, w{}; };

template <typename T> using Vec     = std::vector<T>;
template <typename T, usize N> using Arr = std::array<T, N>;
template <typename K, typename V> using Map = std::unordered_map<K, V>;
template <typename K> using Set               = std::unordered_set<K>;
template <typename T> using Ref               = std::shared_ptr<T>;
template <typename T> using Weak              = std::weak_ptr<T>;
template <typename T> using Scope             = std::unique_ptr<T>;

template <typename T, typename... Args>
[[nodiscard]] constexpr Ref<T> MakeRef(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
[[nodiscard]] constexpr Scope<T> MakeScope(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Time
using Clock     = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;
using Duration  = Clock::duration;

inline f32 durationToSeconds(Duration d) {
    return std::chrono::duration<f32>(d).count();
}

// ID types
struct EntityID    { u64 value{0}; bool operator==(const EntityID& o) const noexcept { return value == o.value; } bool operator!=(const EntityID& o) const noexcept { return value != o.value; } };
struct ComponentID { u64 value{0}; };
struct ResourceID  { u64 value{0}; };
struct AssetID     { u64 value{0}; };

} // namespace pk

namespace std {
template<> struct hash<pk::EntityID> {
    size_t operator()(const pk::EntityID& e) const noexcept { return hash<pk::u64>{}(e.value); }
};
}
