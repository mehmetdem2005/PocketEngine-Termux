// ============================================================
//  pocket/engine/platform/window.h
//  GLFW-based window, Termux:X11 compatible
// ============================================================
#pragma once

#include "core/types.h"
#include "core/event.h"

struct GLFWwindow;

namespace pk::platform {

struct WindowDesc {
    i32 width       = 1280;
    i32 height      = 720;
    String title    = "PocketEngine";
    bool fullscreen = false;
    bool resizable  = true;
    bool visible    = true;
    // Landscape hint - on Termux:X11 the WM usually respects requested size
    bool landscape  = true;
    i32  glMajor    = 3;
    i32  glMinor    = 0;
};

class Window {
public:
    Window() = default;
    ~Window();

    bool create(const WindowDesc& desc) noexcept;
    void destroy() noexcept;
    bool shouldClose() const noexcept;
    void pollEvents() noexcept;
    void swapBuffers() noexcept;
    void makeCurrent() noexcept;

    Vec2<i32> size()       const noexcept;
    Vec2<i32> framebufferSize() const noexcept;
    f32       aspect()     const noexcept;

    GLFWwindow* handle() const noexcept { return m_handle; }

    // Input state
    bool keyPressed(i32 key) const noexcept;
    bool mousePressed(i32 button) const noexcept;
    Vec2<f32> mousePos() const noexcept;

    // Set landscape-only hint (Android: call before create)
    void setLandscapeOnly(bool v) noexcept { m_desc.landscape = v; }

private:
    GLFWwindow* m_handle{nullptr};
    WindowDesc  m_desc;
};

} // namespace pk::platform
