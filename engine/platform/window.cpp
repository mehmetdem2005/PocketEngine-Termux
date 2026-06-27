// ============================================================
//  pocket/engine/platform/window.cpp
// ============================================================
#include "platform/window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef POCKET_GLES3
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#else
#include <GL/gl.h>
#endif

#include "core/log.h"

namespace pk::platform {

namespace {
Window* g_windowForCallbacks = nullptr;
}

Window::~Window() { destroy(); }

bool Window::create(const WindowDesc& desc) noexcept {
    m_desc = desc;

    if (!glfwInit()) {
        PK_LOG_ERROR("Window", "glfwInit() failed");
        return false;
    }

    // Hint OpenGL ES 3.0 on Termux:X11 (Mali/Adreno supports it)
#ifdef POCKET_GLES3
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE,   desc.visible   ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // 4x MSAA

    // Landscape enforcement: pick larger dim as width
    i32 w = desc.width, h = desc.height;
    if (desc.landscape && h > w) std::swap(w, h);

    GLFWmonitor* mon = desc.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_handle = glfwCreateWindow(w, h, desc.title.c_str(), mon, nullptr);
    if (!m_handle) {
        PK_LOG_ERROR("Window", "glfwCreateWindow(%dx%d) failed", w, h);
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_handle);
    // On Termux:X11, OpenGL ES functions are statically linked via libGLESv3.so
    // No GLAD loader needed.
    glfwSwapInterval(1); // vsync

    // Callbacks
    g_windowForCallbacks = this;
    glfwSetFramebufferSizeCallback(m_handle, [](GLFWwindow*, int w, int h) {
        glViewport(0, 0, w, h);
        event::bus().publish(events::WindowResize{w, h});
    });
    glfwSetKeyCallback(m_handle, [](GLFWwindow*, int key, int sc, int act, int mods) {
        if (act == GLFW_PRESS)   event::bus().publish(events::KeyPress{key, sc, mods});
        if (act == GLFW_RELEASE) event::bus().publish(events::KeyRelease{key, sc, mods});
    });
    glfwSetCursorPosCallback(m_handle, [](GLFWwindow*, double x, double y) {
        event::bus().publish(events::MouseMove{static_cast<f32>(x), static_cast<f32>(y)});
    });
    glfwSetMouseButtonCallback(m_handle, [](GLFWwindow*, int btn, int act, int) {
        double x, y; glfwGetCursorPos(glfwGetCurrentContext(), &x, &y);
        if (act == GLFW_PRESS)
            event::bus().publish(events::MousePress{btn, (f32)x, (f32)y});
        else
            event::bus().publish(events::MouseRelease{btn, (f32)x, (f32)y});
    });
    glfwSetErrorCallback([](int code, const char* msg) {
        PK_LOG_ERROR("GLFW", "[%d] %s", code, msg);
    });

    int fbw, fbh;
    glfwGetFramebufferSize(m_handle, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    PK_LOG_INFO("Window", "Window created: %dx%d (fb=%dx%d)", w, h, fbw, fbh);
    return true;
}

void Window::destroy() noexcept {
    if (m_handle) {
        glfwDestroyWindow(m_handle);
        m_handle = nullptr;
        glfwTerminate();
    }
}

bool Window::shouldClose() const noexcept {
    return glfwWindowShouldClose(m_handle) == GLFW_TRUE;
}

void Window::pollEvents()  noexcept { glfwPollEvents(); }
void Window::swapBuffers() noexcept { glfwSwapBuffers(m_handle); }
void Window::makeCurrent() noexcept { glfwMakeContextCurrent(m_handle); }

Vec2<i32> Window::size() const noexcept {
    int w, h; glfwGetWindowSize(m_handle, &w, &h);
    return {w, h};
}
Vec2<i32> Window::framebufferSize() const noexcept {
    int w, h; glfwGetFramebufferSize(m_handle, &w, &h);
    return {w, h};
}
f32 Window::aspect() const noexcept {
    auto s = size();
    return s.x ? static_cast<f32>(s.x) / s.y : 1.0f;
}

bool Window::keyPressed(i32 key) const noexcept {
    return glfwGetKey(m_handle, key) == GLFW_PRESS;
}
bool Window::mousePressed(i32 button) const noexcept {
    return glfwGetMouseButton(m_handle, button) == GLFW_PRESS;
}
Vec2<f32> Window::mousePos() const noexcept {
    double x, y; glfwGetCursorPos(m_handle, &x, &y);
    return {(f32)x, (f32)y};
}

} // namespace pk::platform
