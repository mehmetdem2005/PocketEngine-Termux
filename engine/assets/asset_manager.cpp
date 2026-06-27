// ============================================================
//  pocket/engine/assets/asset_manager.cpp
// ============================================================
#include "assets/asset_manager.h"
#include "core/log.h"

#include <sys/stat.h>
#include <fstream>

namespace pk::asset {

namespace {
u64 fileHash(const String& path) noexcept {
    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;
    u64 h = 1469598103934665603ULL;  // FNV offset
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        for (std::streamsize i = 0; i < f.gcount(); ++i) {
            h ^= (u8)buf[i];
            h *= 1099511628211ULL;
        }
    }
    return h;
}

bool fileMtime(const String& path, TimePoint& out) noexcept {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) return false;
    out = Clock::time_point(std::chrono::seconds(st.st_mtime));
    return true;
}

u64 fileSize(const String& path) noexcept {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) return 0;
    return st.st_size;
}
} // namespace

AssetManager& assets() noexcept {
    static AssetManager a;
    return a;
}

AssetID AssetManager::registerAsset(const String& path, AssetType type, u64 size, u64 hash) noexcept {
    AssetID id{m_nextID++};
    Asset a;
    a.id = id;
    a.path = path;
    a.type = type;
    a.sizeBytes = size;
    a.hash = hash;
    a.loaded = false;
    fileMtime(path, a.mtime);

    // Extract name from path
    auto slash = path.find_last_of('/');
    a.name = (slash != String::npos) ? path.substr(slash + 1) : path;

    m_assets.push_back(a);
    m_pathToID[path] = id;
    return id;
}

AssetID AssetManager::loadTexture(const String& path, bool smooth) noexcept {
    auto it = m_pathToID.find(path);
    if (it != m_pathToID.end()) {
        auto* a = &m_assets[it->second.value - 1];
        if (a->loaded) return a->id;
    }

    String full = m_root.empty() ? path : (m_root + "/" + path);
    int w, h, ch;
    stbi_set_flip_vertically_on_load(1);
    u8* pixels = stbi_load(full.c_str(), &w, &h, &ch, 0);
    if (!pixels) {
        PK_LOG_ERROR("Asset", "Failed to load texture: %s", full.c_str());
        return AssetID{0};
    }

    u32 handle = rnd::renderer().uploadTexture(pixels, w, h, ch, smooth);
    stbi_image_free(pixels);

    AssetID id = registerAsset(path, AssetType::Texture, fileSize(full), fileHash(full));
    auto& a = m_assets[id.value - 1];
    a.loaded = true;
    a.handle = handle;
    PK_LOG_INFO("Asset", "Loaded texture '%s' (%dx%d, %dch, GPU=%u)",
                path.c_str(), w, h, ch, handle);
    return id;
}

AssetID AssetManager::loadTextureFromMemory(const u8* data, usize size,
                                             const String& name, bool smooth) noexcept {
    int w, h, ch;
    u8* pixels = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &ch, 0);
    if (!pixels) return AssetID{0};

    u32 handle = rnd::renderer().uploadTexture(pixels, w, h, ch, smooth);
    stbi_image_free(pixels);

    AssetID id = registerAsset(name, AssetType::Texture, size, 0);
    auto& a = m_assets[id.value - 1];
    a.loaded = true;
    a.handle = handle;
    return id;
}

rnd::Texture* AssetManager::getTexture(AssetID id) noexcept {
    if (id.value == 0 || id.value > m_assets.size()) return nullptr;
    auto& a = m_assets[id.value - 1];
    if (!a.loaded) return nullptr;
    static thread_local rnd::Texture t;
    t.id = a.handle;
    return &t;
}

rnd::Texture* AssetManager::getTextureByPath(const String& path) noexcept {
    auto it = m_pathToID.find(path);
    if (it == m_pathToID.end()) return nullptr;
    return getTexture(it->second);
}

void AssetManager::unload(AssetID id) noexcept {
    if (id.value == 0 || id.value > m_assets.size()) return;
    auto& a = m_assets[id.value - 1];
    if (a.loaded && a.handle) {
        rnd::renderer().deleteTexture(a.handle);
        a.handle = 0;
        a.loaded = false;
    }
}

void AssetManager::unloadAll() noexcept {
    for (auto& a : m_assets) {
        if (a.loaded && a.handle) {
            rnd::renderer().deleteTexture(a.handle);
            a.handle = 0;
            a.loaded = false;
        }
    }
}

usize AssetManager::checkHotReload() noexcept {
    usize reloaded = 0;
    for (auto& a : m_assets) {
        if (!a.loaded) continue;
        TimePoint mt;
        if (!fileMtime(a.path, mt)) continue;
        if (mt > a.mtime) {
            a.mtime = mt;
            // Reload
            if (a.type == AssetType::Texture) {
                rnd::renderer().deleteTexture(a.handle);
                a.handle = 0;
                a.loaded = false;
                loadTexture(a.path);
                reloaded++;
            }
        }
    }
    return reloaded;
}

} // namespace pk::asset
