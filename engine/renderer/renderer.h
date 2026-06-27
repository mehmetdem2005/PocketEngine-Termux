// ============================================================
//  pocket/engine/renderer/renderer.h
//  OpenGL ES 3.0 batched 2D + basic 3D renderer
// ============================================================
#pragma once

#include "core/types.h"
#include "core/math.h"
#include "core/alloc.h"

#ifdef POCKET_GLES3
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace pk::rnd {

constexpr u32 MAX_BATCH_QUADS   = 4096;
constexpr u32 MAX_VERTICES      = MAX_BATCH_QUADS * 4;
constexpr u32 MAX_INDICES       = MAX_BATCH_QUADS * 6;
constexpr u32 MAX_TEXTURE_SLOTS = 16;

struct Vertex2D {
    math::Vec3  pos;
    math::Vec4  color;
    math::Vec2  uv;
    f32         texIndex;  // 0 = solid color
};

struct Texture {
    u32     id{0};
    i32     width{0}, height{0};
    i32     channels{0};
    String  path;
    bool    smooth{true};
};

struct Shader {
    u32     program{0};
    String  name;
    String  vertexSrc;
    String  fragmentSrc;

    void use() const noexcept { glUseProgram(program); }
    void setMat4(const char* name, const math::Mat4& m) const noexcept;
    void setInt(const char* name, i32 v) const noexcept;
    void setFloat(const char* name, f32 v) const noexcept;
    void setVec4(const char* name, const math::Vec4& v) const noexcept;
};

class Renderer2D {
public:
    Renderer2D() = default;
    ~Renderer2D();

    bool init(i32 fbWidth, i32 fbHeight) noexcept;
    void shutdown() noexcept;

    void beginScene(const math::Mat4& viewProj) noexcept;
    void endScene() noexcept;
    void flush() noexcept;

    // Primitives
    void drawQuad(f32 x, f32 y, f32 w, f32 h, const math::Vec4& color) noexcept;
    void drawSprite(u32 textureID, f32 x, f32 y, f32 w, f32 h,
                    f32 u0=0, f32 v0=0, f32 u1=1, f32 v1=1,
                    const math::Vec4& color = {1,1,1,1}) noexcept;
    void drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const math::Vec4& color) noexcept;
    void drawCircle(f32 cx, f32 cy, f32 r, const math::Vec4& color, u32 segments = 32) noexcept;

    void setViewport(i32 x, i32 y, i32 w, i32 h) noexcept;
    void setClearColor(f32 r, f32 g, f32 b, f32 a=1) noexcept;
    void clear() noexcept;

    // Texture management
    u32  uploadTexture(const u8* pixels, i32 w, i32 h, i32 channels,
                       bool smooth = true, bool repeat = false) noexcept;
    void deleteTexture(u32 id) noexcept;

    // Stats
    struct Stats {
        u32 drawCalls{0};
        u32 vertices{0};
        u32 quads{0};
        u32 textures{0};
    };
    Stats stats() const noexcept { return m_stats; }
    void  resetStats() noexcept { m_stats = Stats{}; }

private:
    void initShaders() noexcept;
    void initBuffers() noexcept;

    GLuint m_vao{0}, m_vbo{0}, m_ibo{0};
    GLuint m_lineVAO{0}, m_lineVBO{0};
    Shader m_batchShader;
    Shader m_lineShader;

    Vertex2D   m_vertices[MAX_VERTICES];
    u32        m_vertexCount{0};
    u32        m_indexCount{0};

    u32        m_textureSlots[MAX_TEXTURE_SLOTS];
    u32        m_textureSlotCount{0};

    math::Mat4 m_viewProj;

    Stats      m_stats;
    bool       m_inited{false};

    static constexpr u32 MAX_LINE_VERTICES = 4096;
    Vertex2D             m_lineVertices[MAX_LINE_VERTICES];
    u32                  m_lineVertexCount{0};
};

// Global renderer (engine owns one)
Renderer2D& renderer() noexcept;

} // namespace pk::rnd
