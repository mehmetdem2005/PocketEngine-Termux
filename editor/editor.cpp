// ============================================================
//  pocket/editor/editor.cpp
//  Unity-style landscape editor - main entry
// ============================================================
#include "editor.h"
#include "panels/hierarchy.h"
#include "panels/inspector.h"
#include "panels/scene_view.h"
#include "panels/console.h"
#include "panels/profiler.h"
#include "panels/asset_browser.h"

#include "core/log.h"
#include "core/thread_pool.h"
#include "core/alloc.h"
#include "core/ecs.h"
#include "core/math.h"
#include "platform/window.h"
#include "renderer/renderer.h"
#include "ui/imgui_backend.h"
#include "assets/asset_manager.h"
#include "db/database.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <chrono>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>

namespace pk::editor {

Editor::~Editor() { shutdown(); }

bool Editor::init(platform::Window* window) noexcept {
    m_window = window;

    if (!ui::initImGui(window->handle())) {
        PK_LOG_ERROR("Editor", "ImGui init failed");
        return false;
    }

    // Init renderer
    auto fb = window->framebufferSize();
    rnd::renderer().init(fb.x, fb.y);

    // Init DB - use POCKET_DATA env var, fallback to ~/.pocket_data
    auto& db = pk::db::engineDB();
    const char* dataEnv = std::getenv("POCKET_DATA");
    String dataDir = dataEnv ? dataEnv : String(getenv("HOME")) + "/.pocket_data";
    // Ensure directory exists
    ::mkdir(dataDir.c_str(), 0755);
    String dbPath = dataDir + "/engine.db";
    if (!db.open(dbPath)) {
        PK_LOG_WARN("Editor", "Failed to open DB at %s - continuing without DB", dbPath.c_str());
    } else {
        db.initEngineSchema();
    }

    // Init asset manager root
    const char* assetEnv = std::getenv("POCKET_ASSETS");
    String assetDir = assetEnv ? assetEnv : String(getenv("HOME")) + "/.pocket_assets";
    ::mkdir(assetDir.c_str(), 0755);
    pk::asset::assets().setRoot(assetDir);

    // Default scene with a camera + test sprite
    auto cam = m_scene.world.create();
    m_scene.world.add<ecs::Tag>(cam, ecs::Tag{"Main Camera"});
    m_scene.world.add<ecs::Camera>(cam, ecs::Camera{});

    auto sprite1 = m_scene.world.create();
    m_scene.world.add<ecs::Tag>(sprite1, ecs::Tag{"Player"});
    m_scene.world.add<ecs::Transform>(sprite1, ecs::Transform{0, 0, 0, 0,0,0, 1,1,1});
    m_scene.world.add<ecs::Sprite>(sprite1, ecs::Sprite{0, 1, 1, 0,0,1,1, {1,1,1,1}});

    PK_LOG_INFO("Editor", "Editor initialized");
    return true;
}

void Editor::shutdown() noexcept {
    ui::shutdownImGui();
    rnd::renderer().shutdown();
    pk::db::engineDB().close();
}

void Editor::run() noexcept {
    auto& window = *m_window;
    auto& rdr    = rnd::renderer();

    f32 camX = 0, camY = 0;
    f32 camZoom = 5.0f;

    PK_LOG_INFO("Editor", "run() entered - starting main loop");
    int frameCount = 0;

    while (!window.shouldClose()) {
        window.pollEvents();
        PK_LOG_DEBUG("Editor", "frame %d: after pollEvents", frameCount);

        auto fb = window.framebufferSize();
        rdr.setViewport(0, 0, fb.x, fb.y);
        rdr.setClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        rdr.clear();
        PK_LOG_DEBUG("Editor", "frame %d: after clear", frameCount);

        // Begin ImGui
        ui::beginImGuiFrame();
        PK_LOG_DEBUG("Editor", "frame %d: after beginImGuiFrame", frameCount);

        // TEST: minimal - just show a demo window to verify ImGui works
        ImGui::ShowDemoWindow();
        PK_LOG_DEBUG("Editor", "frame %d: after ShowDemoWindow", frameCount);

        // Skip the rest for now - isolate the crash
#if 0
        dockspaceLayout();
        drawMainMenuBar();
        drawToolbar();

        if (m_showHierarchy)      drawHierarchyPanel();
        if (m_showInspector)      drawInspectorPanel();
        if (m_showSceneView)      drawSceneView();
        if (m_showGameView)       drawGameView();
        if (m_showAssetBrowser)   drawAssetBrowserPanel();
        if (m_showConsole)        drawConsolePanel();
        if (m_showProfiler)       drawProfilerPanel();
        drawStatusBar();

        if (m_showImGuiDemo) ImGui::ShowDemoWindow();

        // Game view rendering (separate FBO would be ideal; we share main here)
        // Compute camera matrix
        f32 aspect = (f32)fb.x / (f32)fb.y;
        auto vp = math::ortho(-camZoom * aspect + camX, camZoom * aspect + camX,
                              -camZoom + camY, camZoom + camY,
                              -100, 100);

        rdr.beginScene(vp);
        // Render all sprites
        m_scene.world.each<ecs::Transform, ecs::Sprite>(
            [&](EntityID, ecs::Transform& t, ecs::Sprite& s) {
                rdr.drawSprite(s.textureID, t.x - s.width * 0.5f * t.sx,
                               t.y - s.height * 0.5f * t.sy,
                               s.width * t.sx, s.height * t.sy,
                               s.u0, s.v0, s.u1, s.v1,
                               math::Vec4(s.color[0], s.color[1], s.color[2], s.color[3]));
            });
        rdr.endScene();

        // Camera controls
        if (window.keyPressed(262)) camX += 0.05f;   // right
        if (window.keyPressed(263)) camX -= 0.05f;   // left
        if (window.keyPressed(265)) camY += 0.05f;   // up
        if (window.keyPressed(264)) camY -= 0.05f;   // down
#endif

        ui::endImGuiFrame();
        PK_LOG_DEBUG("Editor", "frame %d: after endImGuiFrame", frameCount);

        window.swapBuffers();
        PK_LOG_DEBUG("Editor", "frame %d: after swapBuffers", frameCount);
        frameCount++;

        // Safety: stop after 5 frames to see logs
        if (frameCount >= 5) {
            PK_LOG_INFO("Editor", "Survived 5 frames - success!");
            break;
        }
    }
}

void Editor::dockspaceLayout() noexcept {
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
                           | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                           | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("##Dockspace", nullptr, flags);
    ImGui::PopStyleVar(3);

    m_dockspaceId = ImGui::GetID("PocketDockspace");
    if (!m_dockspaceBuilt) {
        m_dockspaceBuilt = true;
        ImGui::DockBuilderRemoveNode(m_dockspaceId);
        ImGui::DockBuilderAddNode(m_dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_dockspaceId, vp->WorkSize);

        // Unity-like layout for landscape
        ImGuiID main = m_dockspaceId;
        ImGuiID left, center, right, bottom;
        ImGui::DockBuilderSplitNode(main, ImGuiDir_Left,   0.16f, &left, &center);
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.22f, &right, &center);
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,  0.30f, &bottom, &center);

        ImGui::DockBuilderDockWindow("Hierarchy", left);
        ImGui::DockBuilderDockWindow("Scene View", center);
        ImGui::DockBuilderDockWindow("Inspector", right);
        ImGui::DockBuilderDockWindow("Console", bottom);
        ImGui::DockBuilderDockWindow("Project", bottom);
        ImGui::DockBuilderDockWindow("Profiler", bottom);
        ImGui::DockBuilderFinish(m_dockspaceId);
    }

    ImGui::DockSpace(m_dockspaceId, ImVec2(0,0),
                     ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

void Editor::drawMainMenuBar() noexcept {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) newScene();
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) openScene();
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) saveScene();
            ImGui::Separator();
            if (ImGui::MenuItem("Build & Run", "F5")) buildAndRun();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences...")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Create Empty")) {
                auto e = m_scene.world.create();
                m_scene.world.add<ecs::Tag>(e, ecs::Tag{"GameObject"});
                m_scene.world.add<ecs::Transform>(e, ecs::Transform{});
            }
            if (ImGui::MenuItem("Create Sprite")) {
                auto e = m_scene.world.create();
                m_scene.world.add<ecs::Tag>(e, ecs::Tag{"Sprite"});
                m_scene.world.add<ecs::Transform>(e, ecs::Transform{});
                m_scene.world.add<ecs::Sprite>(e, ecs::Sprite{});
            }
            if (ImGui::MenuItem("Create Camera")) {
                auto e = m_scene.world.create();
                m_scene.world.add<ecs::Tag>(e, ecs::Tag{"Camera"});
                m_scene.world.add<ecs::Camera>(e, ecs::Camera{});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy",     nullptr, &m_showHierarchy);
            ImGui::MenuItem("Inspector",     nullptr, &m_showInspector);
            ImGui::MenuItem("Scene View",    nullptr, &m_showSceneView);
            ImGui::MenuItem("Game View",     nullptr, &m_showGameView);
            ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
            ImGui::MenuItem("Console",       nullptr, &m_showConsole);
            ImGui::MenuItem("Profiler",      nullptr, &m_showProfiler);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showImGuiDemo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Editor::drawToolbar() noexcept {
    ImGui::Begin("##Toolbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::Button("Play",  ImVec2(60, 26));
    ImGui::SameLine();
    ImGui::Button("Pause", ImVec2(60, 26));
    ImGui::SameLine();
    ImGui::Button("Step",  ImVec2(60, 26));
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();
    ImGui::Button("Save",  ImVec2(60, 26));
    ImGui::SameLine();
    ImGui::Button("Build", ImVec2(60, 26));
    ImGui::End();
}

void Editor::drawHierarchyPanel() noexcept {
    if (!ImGui::Begin("Hierarchy", &m_showHierarchy)) {
        ImGui::End();
        return;
    }
    // Header buttons
    if (ImGui::Button("+ Add")) ImGui::OpenPopup("AddMenu");
    if (ImGui::BeginPopup("AddMenu")) {
        if (ImGui::MenuItem("Empty")) {
            auto e = m_scene.world.create();
            m_scene.world.add<ecs::Tag>(e, ecs::Tag{"GameObject"});
            m_scene.world.add<ecs::Transform>(e, ecs::Transform{});
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%zu entities)", m_scene.world.entityCount());
    ImGui::Separator();

    // Iterate entities that have Tag
    m_scene.world.each<ecs::Tag>([&](EntityID e, ecs::Tag& tag) {
        bool selected = (m_selectedEntity == e);
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                   ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_FramePadding;
        if (selected) flags |= ImGuiTreeNodeFlags_Selected;
        ImGui::TreeNodeEx(reinterpret_cast<void*>(e.value), flags, "%s",
                          tag.name.c_str());
        if (ImGui::IsItemClicked()) m_selectedEntity = e;
    });

    ImGui::End();
}

void Editor::drawInspectorPanel() noexcept {
    if (!ImGui::Begin("Inspector", &m_showInspector)) {
        ImGui::End();
        return;
    }
    if (m_selectedEntity.value == 0) {
        ImGui::TextDisabled("Select an entity");
        ImGui::End();
        return;
    }

    auto* tag = m_scene.world.get<ecs::Tag>(m_selectedEntity);
    if (tag) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s", tag->name.c_str());
        if (ImGui::InputText("Name", buf, sizeof(buf))) {
            tag->name = buf;
        }
    }

    if (auto* t = m_scene.world.get<ecs::Transform>(m_selectedEntity)) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("X", &t->x, 0.1f);
            ImGui::DragFloat("Y", &t->y, 0.1f);
            ImGui::DragFloat("Z", &t->z, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat("Rot X", &t->rx, 1.0f);
            ImGui::DragFloat("Rot Y", &t->ry, 1.0f);
            ImGui::DragFloat("Rot Z", &t->rz, 1.0f);
            ImGui::Separator();
            ImGui::DragFloat("Scale X", &t->sx, 0.1f, 0.01f, 100.0f);
            ImGui::DragFloat("Scale Y", &t->sy, 0.1f, 0.01f, 100.0f);
            ImGui::DragFloat("Scale Z", &t->sz, 0.1f, 0.01f, 100.0f);
        }
    }

    if (auto* s = m_scene.world.get<ecs::Sprite>(m_selectedEntity)) {
        if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragInt("Tex ID", reinterpret_cast<i32*>(&s->textureID));
            ImGui::DragFloat("Width",  &s->width, 0.1f);
            ImGui::DragFloat("Height", &s->height, 0.1f);
            ImGui::ColorEdit4("Tint", s->color);
        }
    }

    if (auto* c = m_scene.world.get<ecs::Camera>(m_selectedEntity)) {
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("Ortho Size", &c->orthoSize, 0.1f);
            ImGui::DragFloat("Near", &c->near_, 0.01f);
            ImGui::DragFloat("Far",  &c->far_,  0.1f);
            ImGui::ColorEdit4("Clear", c->clearColor);
            ImGui::Checkbox("Active", &c->active);
        }
    }

    if (ImGui::Button("+ Add Component")) ImGui::OpenPopup("AddComp");
    if (ImGui::BeginPopup("AddComp")) {
        if (ImGui::MenuItem("Transform")) m_scene.world.add<ecs::Transform>(m_selectedEntity, ecs::Transform{});
        if (ImGui::MenuItem("Sprite"))    m_scene.world.add<ecs::Sprite>(m_selectedEntity, ecs::Sprite{});
        if (ImGui::MenuItem("Camera"))    m_scene.world.add<ecs::Camera>(m_selectedEntity, ecs::Camera{});
        if (ImGui::MenuItem("RigidBody")) m_scene.world.add<ecs::RigidBody>(m_selectedEntity, ecs::RigidBody{});
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Editor::drawSceneView() noexcept {
    if (!ImGui::Begin("Scene View", &m_showSceneView)) {
        ImGui::End();
        return;
    }
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Scene View (%dx%d)",
                       (int)size.x, (int)size.y);
    ImGui::Separator();
    // 2D grid
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
    dl->AddRectFilled(p0, p1, IM_COL32(35, 35, 38, 255));
    // Grid lines
    int step = 50;
    ImU32 col = IM_COL32(60, 60, 65, 255);
    for (float x = p0.x; x < p1.x; x += step) {
        dl->AddLine(ImVec2(x, p0.y), ImVec2(x, p1.y), col);
    }
    for (float y = p0.y; y < p1.y; y += step) {
        dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), col);
    }
    // Center crosshair
    float cx = (p0.x + p1.x) * 0.5f;
    float cy = (p0.y + p1.y) * 0.5f;
    dl->AddLine(ImVec2(cx - 10, cy), ImVec2(cx + 10, cy), IM_COL32(255, 200, 0, 255), 2);
    dl->AddLine(ImVec2(cx, cy - 10), ImVec2(cx, cy + 10), IM_COL32(255, 200, 0, 255), 2);

    ImGui::Dummy(size);
    ImGui::End();
}

void Editor::drawGameView() noexcept {
    if (!ImGui::Begin("Game View", &m_showGameView)) {
        ImGui::End();
        return;
    }
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Game View (Live)");
    ImGui::Separator();
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
    dl->AddRectFilled(p0, p1, IM_COL32(15, 15, 18, 255));
    // Render sprite entities via ImDrawList as preview
    m_scene.world.each<ecs::Transform, ecs::Sprite>(
        [&](EntityID, ecs::Transform& t, ecs::Sprite& s) {
            float cx = p0.x + size.x * 0.5f + t.x * 30.0f;
            float cy = p0.y + size.y * 0.5f + t.y * 30.0f;
            float w  = s.width * 30.0f * t.sx;
            float h  = s.height * 30.0f * t.sy;
            ImU32 col = IM_COL32((int)(s.color[0]*255), (int)(s.color[1]*255),
                                 (int)(s.color[2]*255), (int)(s.color[3]*255));
            dl->AddRectFilled(ImVec2(cx - w*0.5f, cy - h*0.5f),
                              ImVec2(cx + w*0.5f, cy + h*0.5f), col);
        });
    ImGui::Dummy(size);
    ImGui::End();
}

void Editor::drawProjectPanel() noexcept {
    // merged into AssetBrowser panel
}

void Editor::drawAssetBrowserPanel() noexcept {
    if (!ImGui::Begin("Project", &m_showAssetBrowser)) {
        ImGui::End();
        return;
    }
    // Two-column: tree (left) + grid (right)
    float leftW = 200;
    ImGui::BeginChild("Dirs", ImVec2(leftW, 0), true);
    ImGui::Text("Assets");
    ImGui::Separator();
    ImGui::TextDisabled("- textures/");
    ImGui::TextDisabled("- shaders/");
    ImGui::TextDisabled("- audio/");
    ImGui::TextDisabled("- scenes/");
    ImGui::TextDisabled("- scripts/");
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Files", ImVec2(0, 0), true);
    if (ImGui::Button("Import...")) {}
    ImGui::Separator();
    // Show grid of assets
    auto& assets = pk::asset::assets().assets();
    float sz = 64;
    float pad = 8;
    float avail = ImGui::GetContentRegionAvail().x;
    int perRow = (int)(avail / (sz + pad));
    if (perRow < 1) perRow = 1;
    for (usize i = 0; i < assets.size(); ++i) {
        if (i % perRow != 0) ImGui::SameLine();
        ImGui::BeginGroup();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(p0, ImVec2(p0.x+sz, p0.y+sz),
                                                  IM_COL32(80, 80, 90, 200));
        ImGui::Dummy(ImVec2(sz, sz));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + sz);
        ImGui::TextUnformatted(assets[i].name.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();
    }
    ImGui::EndChild();
    ImGui::End();
}

void Editor::drawConsolePanel() noexcept {
    if (!ImGui::Begin("Console", &m_showConsole)) {
        ImGui::End();
        return;
    }
    if (ImGui::Button("Clear")) {}
    ImGui::SameLine();
    static bool showInfo = true, showWarn = true, showError = true;
    ImGui::Checkbox("Info",  &showInfo);  ImGui::SameLine();
    ImGui::Checkbox("Warn",  &showWarn);  ImGui::SameLine();
    ImGui::Checkbox("Error", &showError);
    ImGui::Separator();
    ImGui::BeginChild("log");
    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "[INFO]  PocketEngine v0.1.0");
    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "[INFO]  Renderer2D ready");
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[WARN]  loadScene: JSON parsing stub");
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[ERROR] (no errors yet)");
    ImGui::EndChild();
    ImGui::End();
}

void Editor::drawProfilerPanel() noexcept {
    if (!ImGui::Begin("Profiler", &m_showProfiler)) {
        ImGui::End();
        return;
    }
    auto& r = rnd::renderer();
    auto stats = r.stats();
    ImGui::Text("Draw Calls: %u", stats.drawCalls);
    ImGui::Text("Vertices:   %u", stats.vertices);
    ImGui::Text("Quads:      %u", stats.quads);
    ImGui::Text("Textures:   %u", stats.textures);
    ImGui::Separator();
    auto& a = pk::alloc::stats();
    ImGui::Text("Allocations:     %llu", (unsigned long long)a.totalAllocations.load());
    ImGui::Text("Deallocations:   %llu", (unsigned long long)a.totalDeallocations.load());
    ImGui::Text("Bytes alloc:     %llu", (unsigned long long)a.bytesAllocated.load());
    ImGui::Text("Bytes dealloc:   %llu", (unsigned long long)a.bytesDeallocated.load());
    ImGui::Text("Peak bytes:      %llu", (unsigned long long)a.peakBytes.load());
    ImGui::Separator();
    ImGui::Text("Threads: %zu", pk::globalThreadPool().workerCount());
    ImGui::Text("Frame arena used: %zu / %zu",
                pk::alloc::frameArena().used(),
                pk::alloc::frameArena().capacity());
    ImGui::End();
}

void Editor::drawStatusBar() noexcept {
    ImGui::Begin("##StatusBar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("PocketEngine v0.1.0 | ");
    ImGui::SameLine(0, 0);
    ImGui::Text("Scene: %s | ", m_scene.name.c_str());
    ImGui::SameLine(0, 0);
    ImGui::Text("Entities: %zu | ", m_scene.world.entityCount());
    ImGui::SameLine(0, 0);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}

void Editor::newScene() noexcept {
    m_scene.name = "Untitled";
    m_scene.paused = false;
    m_scene.timeScale = 1.0f;
    m_scene.world.clear();
    PK_LOG_INFO("Editor", "New scene created");
}

void Editor::openScene() noexcept {
    PK_LOG_INFO("Editor", "Open scene (TODO: file picker)");
}

void Editor::saveScene() noexcept {
    pk::scene::saveScene(m_scene, "/data/data/com.termux/files/home/pocket_data/scene.json");
}

void Editor::buildAndRun() noexcept {
    PK_LOG_INFO("Editor", "Build & Run (TODO: bundle assets + launch runtime)");
}

} // namespace pk::editor
