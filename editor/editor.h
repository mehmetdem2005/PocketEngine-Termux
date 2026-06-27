// ============================================================
//  pocket/editor/editor.h
//  Unity-style landscape editor
// ============================================================
#pragma once

#include "core/types.h"
#include "core/scene.h"
#include "platform/window.h"

namespace pk::editor {

class Editor {
public:
    Editor() = default;
    ~Editor();

    bool init(platform::Window* window) noexcept;
    void run() noexcept;
    void shutdown() noexcept;

private:
    void drawMainMenuBar() noexcept;
    void drawToolbar() noexcept;
    void drawHierarchyPanel() noexcept;
    void drawInspectorPanel() noexcept;
    void drawSceneView() noexcept;
    void drawGameView() noexcept;
    void drawProjectPanel() noexcept;
    void drawConsolePanel() noexcept;
    void drawProfilerPanel() noexcept;
    void drawAssetBrowserPanel() noexcept;
    void drawStatusBar() noexcept;

    void newScene() noexcept;
    void openScene() noexcept;
    void saveScene() noexcept;
    void buildAndRun() noexcept;

    void dockspaceLayout() noexcept;

    platform::Window*       m_window{nullptr};
    scene::Scene            m_scene;
    EntityID                m_selectedEntity{0};
    bool                    m_showImGuiDemo{false};
    bool                    m_showProfiler{true};
    bool                    m_showConsole{true};
    bool                    m_showHierarchy{true};
    bool                    m_showInspector{true};
    bool                    m_showAssetBrowser{true};
    bool                    m_showSceneView{true};
    bool                    m_showGameView{true};
    bool                    m_dockspaceBuilt{false};
    u32                     m_dockspaceId{0};
};

} // namespace pk::editor
