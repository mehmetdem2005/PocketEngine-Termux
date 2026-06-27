// ============================================================
//  pocket/engine/core/math.h
//  Lightweight math (use GLM for heavy lifting)
// ============================================================
#pragma once

#include "core/types.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace pk::math {

using Vec2  = glm::vec2;
using Vec3  = glm::vec3;
using Vec4  = glm::vec4;
using Mat3  = glm::mat3;
using Mat4  = glm::mat4;
using Quat  = glm::quat;

constexpr f32 PI       = 3.14159265358979323846f;
constexpr f32 TWO_PI   = 2.0f * PI;
constexpr f32 HALF_PI  = 0.5f * PI;
constexpr f32 DEG2RAD  = PI / 180.0f;
constexpr f32 RAD2DEG  = 180.0f / PI;

[[nodiscard]] inline f32 clamp(f32 v, f32 lo, f32 hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}
[[nodiscard]] inline f32 lerp(f32 a, f32 b, f32 t) noexcept {
    return a + (b - a) * t;
}
[[nodiscard]] inline f32 smoothstep(f32 e0, f32 e1, f32 x) noexcept {
    f32 t = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

[[nodiscard]] inline Mat4 ortho(f32 l, f32 r, f32 b, f32 t, f32 zn, f32 zf) noexcept {
    return glm::ortho(l, r, b, t, zn, zf);
}
[[nodiscard]] inline Mat4 perspective(f32 fovY, f32 aspect, f32 zn, f32 zf) noexcept {
    return glm::perspective(fovY, aspect, zn, zf);
}
[[nodiscard]] inline Mat4 translate(f32 x, f32 y, f32 z) noexcept {
    return glm::translate(Mat4(1.0f), Vec3(x, y, z));
}
[[nodiscard]] inline Mat4 scale(f32 x, f32 y, f32 z) noexcept {
    return glm::scale(Mat4(1.0f), Vec3(x, y, z));
}
[[nodiscard]] inline Mat4 rotateY(f32 a) noexcept {
    return glm::rotate(Mat4(1.0f), a, Vec3(0,1,0));
}

} // namespace pk::math
