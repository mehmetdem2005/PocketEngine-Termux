// ============================================================
//  pocket/engine/ui/imgui_backend.cpp
// ============================================================
#include "ui/imgui_backend.h"
#include "core/log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef POCKET_GLES3
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace pk::ui {

bool initImGui(GLFWwindow* window) noexcept {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = "pocket_editor.ini";

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 4.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    applyProTheme();
    setFontScale(1.4f);  // Bigger fonts for phone screen

    const char* glslVer =
#ifdef POCKET_GLES3
        "#version 300 es";
#else
        "#version 330";
#endif

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        PK_LOG_ERROR("ImGui", "ImGui_ImplGlfw_InitForOpenGL failed");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glslVer)) {
        PK_LOG_ERROR("ImGui", "ImGui_ImplOpenGL3_Init failed");
        return false;
    }

    PK_LOG_INFO("ImGui", "ImGui initialized (version %s)", IMGUI_VERSION);
    return true;
}

void shutdownImGui() noexcept {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void beginImGuiFrame() noexcept {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void endImGuiFrame() noexcept {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}

void applyProTheme() noexcept {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(8, 8);
    s.FramePadding  = ImVec2(6, 4);
    s.ItemSpacing   = ImVec2(6, 4);
    s.ItemInnerSpacing = ImVec2(4, 4);
    s.IndentSpacing = 16;
    s.ScrollbarSize = 12;
    s.GrabMinSize   = 8;
    s.WindowBorderSize = 1;
    s.ChildBorderSize  = 1;
    s.PopupBorderSize  = 1;
    s.FrameBorderSize  = 0;
    s.WindowRounding   = 4;
    s.ChildRounding    = 4;
    s.FrameRounding    = 3;
    s.PopupRounding    = 3;
    s.ScrollbarRounding = 6;
    s.GrabRounding     = 3;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // Unity-like dark palette
    ImVec4* c = s.Colors;
    ImVec4 bg   = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
    ImVec4 bg2  = ImVec4(0.20f,  0.20f,  0.20f,  1.00f);
    ImVec4 bg3  = ImVec4(0.25f,  0.25f,  0.25f,  1.00f);
    ImVec4 acc  = ImVec4(0.20f,  0.46f,  0.85f,  1.00f);  // Unity blue
    ImVec4 acc2 = ImVec4(0.30f,  0.60f,  1.00f,  1.00f);
    ImVec4 txt  = ImVec4(0.90f,  0.90f,  0.90f,  1.00f);
    ImVec4 txtD = ImVec4(0.55f,  0.55f,  0.55f,  1.00f);

    c[ImGuiCol_Text]         = txt;
    c[ImGuiCol_TextDisabled] = txtD;
    c[ImGuiCol_WindowBg]     = bg;
    c[ImGuiCol_ChildBg]      = bg;
    c[ImGuiCol_PopupBg]      = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    c[ImGuiCol_Border]       = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    c[ImGuiCol_BorderShadow] = ImVec4(0,0,0,0);
    c[ImGuiCol_FrameBg]      = bg2;
    c[ImGuiCol_FrameBgHovered] = bg3;
    c[ImGuiCol_FrameBgActive]  = acc;
    c[ImGuiCol_TitleBg]        = bg2;
    c[ImGuiCol_TitleBgActive]  = acc;
    c[ImGuiCol_TitleBgCollapsed] = bg2;
    c[ImGuiCol_MenuBarBg]     = bg2;
    c[ImGuiCol_ScrollbarBg]   = bg;
    c[ImGuiCol_ScrollbarGrab]    = bg3;
    c[ImGuiCol_ScrollbarGrabHovered] = acc;
    c[ImGuiCol_ScrollbarGrabActive]  = acc2;
    c[ImGuiCol_CheckMark]     = acc2;
    c[ImGuiCol_SliderGrab]    = acc;
    c[ImGuiCol_SliderGrabActive] = acc2;
    c[ImGuiCol_Button]        = bg3;
    c[ImGuiCol_ButtonHovered] = acc;
    c[ImGuiCol_ButtonActive]  = acc2;
    c[ImGuiCol_Header]        = bg3;
    c[ImGuiCol_HeaderHovered] = acc;
    c[ImGuiCol_HeaderActive]  = acc2;
    c[ImGuiCol_Separator]     = bg3;
    c[ImGuiCol_SeparatorHovered] = acc;
    c[ImGuiCol_SeparatorActive]  = acc2;
    c[ImGuiCol_ResizeGrip]       = bg3;
    c[ImGuiCol_ResizeGripHovered] = acc;
    c[ImGuiCol_ResizeGripActive]  = acc2;
    c[ImGuiCol_Tab]              = bg2;
    c[ImGuiCol_TabHovered]       = acc;
    c[ImGuiCol_TabActive]        = acc;
    c[ImGuiCol_TabUnfocused]     = bg2;
    c[ImGuiCol_TabUnfocusedActive] = bg3;
    c[ImGuiCol_PlotLines]        = acc2;
    c[ImGuiCol_PlotLinesHovered] = acc2;
    c[ImGuiCol_PlotHistogram]    = acc2;
    c[ImGuiCol_PlotHistogramHovered] = acc2;
    c[ImGuiCol_TableHeaderBg]    = bg2;
    c[ImGuiCol_TableBorderStrong] = bg3;
    c[ImGuiCol_TableBorderLight] = bg2;
    c[ImGuiCol_TableRowBg]       = ImVec4(0,0,0,0);
    c[ImGuiCol_TableRowBgAlt]    = ImVec4(1,1,1,0.03f);
    c[ImGuiCol_DragDropTarget]   = acc2;
    c[ImGuiCol_NavHighlight]     = acc2;
    c[ImGuiCol_NavWindowingHighlight] = ImVec4(1,1,1,0.70f);
    c[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.8f,0.8f,0.8f,0.20f);
    c[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.0f,0.0f,0.0f,0.60f);
}

void setFontScale(f32 scale) noexcept {
    ImGuiIO& io = ImGui::GetIO();
    // Use bundled font (no TTF on phone yet)
    io.FontGlobalScale = scale;
}

} // namespace pk::ui
