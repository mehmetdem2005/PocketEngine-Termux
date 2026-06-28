// ============================================================
//  pocket/engine/assets/asset_manager.h
//  Asset loading, caching, hot-reload
// ============================================================
#pragma once

#include "core/types.h"
#include "renderer/renderer.h"

// stb_image is included (with IMPLEMENTATION) only in asset_manager.cpp
// Forward-declare the functions we use:
extern "C" {
typedef unsigned char stbi_uc;
stbi_uc *stbi_load(const char *filename, int *x, int *y, int *channels_in_file, int desired_channels);
stbi_uc *stbi_load_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
void     stbi_image_free(void *retval_from_stbi_load);
void     stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);
}

namespace pk::asset {

enum class AssetType {
    Texture,
    Shader,
    Audio,
    Font,
    Scene,
    Script,
    Other
};

struct Asset {
    AssetID       id;
    String        path;
    String        name;
    AssetType     type;
    u64           sizeBytes{0};
    u64           hash{0};
    bool          loaded{false};
    TimePoint     mtime;
    // Resource handle (texture ID for textures, etc.)
    u32           handle{0};
};

class AssetManager {
public:
    void setRoot(const String& root) noexcept { m_root = root; }
    const String& root() const noexcept { return m_root; }

    AssetID loadTexture(const String& path, bool smooth = true) noexcept;
    AssetID loadTextureFromMemory(const u8* data, usize size, const String& name, bool smooth = true) noexcept;

    rnd::Texture* getTexture(AssetID id) noexcept;
    rnd::Texture* getTextureByPath(const String& path) noexcept;

    void unload(AssetID id) noexcept;
    void unloadAll() noexcept;

    // Hot reload - check mtime of all loaded assets, reload if changed
    usize checkHotReload() noexcept;

    // List of loaded assets (for editor browser)
    const Vec<Asset>& assets() const noexcept { return m_assets; }

private:
    String      m_root;
    Vec<Asset>  m_assets;
    Map<String, AssetID> m_pathToID;
    u64         m_nextID{1};

    AssetID registerAsset(const String& path, AssetType type, u64 size, u64 hash) noexcept;
};

AssetManager& assets() noexcept;

} // namespace pk::asset
