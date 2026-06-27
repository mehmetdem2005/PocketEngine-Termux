// ============================================================
//  pocket/engine/ui/imgui_backend.h
//  ImGui GLFW + OpenGL ES 3 backend wrapper
// ============================================================
#pragma once

#include "core/types.h"

struct GLFWwindow;
struct ImGuiContext;

namespace pk::ui {

bool initImGui(GLFWwindow* glfwWindow) noexcept;
void shutdownImGui() noexcept;
void beginImGuiFrame() noexcept;
void endImGuiFrame() noexcept;

// Apply Unity/Pro dark theme
void applyProTheme() noexcept;

// Set landscape-friendly font scale (DPI)
void setFontScale(f32 scale) noexcept;

} // namespace pk::ui
