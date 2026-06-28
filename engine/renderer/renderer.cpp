// ============================================================
//  pocket/engine/renderer/renderer.cpp
// ============================================================
#include "renderer/renderer.h"
#include "core/log.h"

#include <cstring>

namespace pk::rnd {

// ============================================================
//  Shader helpers
// ============================================================
namespace {

GLuint compileShader(GLenum type, const char* src) noexcept {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        PK_LOG_ERROR("Shader", "Compile error: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint linkProgram(GLuint vs, GLuint fs) noexcept {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        PK_LOG_ERROR("Shader", "Link error: %s", log);
        glDeleteProgram(p);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

constexpr const char* kBatchVertSrc = R"(
    #version 300 es
    precision highp float;
    layout(location=0) in vec3  aPos;
    layout(location=1) in vec4  aColor;
    layout(location=2) in vec2  aUV;
    layout(location=3) in float aTexIndex;
    uniform mat4 uViewProj;
    out vec4  vColor;
    out vec2  vUV;
    out float vTexIndex;
    void main() {
        vColor = aColor;
        vUV = aUV;
        vTexIndex = aTexIndex;
        gl_Position = uViewProj * vec4(aPos, 1.0);
    }
)";

constexpr const char* kBatchFragSrc = R"(
    #version 300 es
    precision highp float;
    in vec4  vColor;
    in vec2  vUV;
    in float vTexIndex;
    out vec4 oColor;
    uniform sampler2D uTextures[16];
    void main() {
        int idx = int(vTexIndex + 0.5);
        vec4 tex = vec4(1.0);
        if (idx > 0) {
            // Manual switch because dynamic indexing is not portable on ES3
            for (int i = 0; i < 16; ++i) {
                if (i == idx) { tex = texture(uTextures[i], vUV); break; }
            }
        }
        oColor = vColor * tex;
    }
)";

constexpr const char* kLineVertSrc = R"(
    #version 300 es
    precision highp float;
    layout(location=0) in vec3 aPos;
    layout(location=1) in vec4 aColor;
    uniform mat4 uViewProj;
    out vec4 vColor;
    void main() {
        vColor = aColor;
        gl_Position = uViewProj * vec4(aPos, 1.0);
    }
)";

constexpr const char* kLineFragSrc = R"(
    #version 300 es
    precision highp float;
    in vec4 vColor;
    out vec4 oColor;
    void main() { oColor = vColor; }
)";

} // namespace

void Shader::setMat4(const char* name, const math::Mat4& m) const noexcept {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
}
void Shader::setInt(const char* name, i32 v) const noexcept {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniform1i(loc, v);
}
void Shader::setFloat(const char* name, f32 v) const noexcept {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniform1f(loc, v);
}
void Shader::setVec4(const char* name, const math::Vec4& v) const noexcept {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniform4f(loc, v.x, v.y, v.z, v.w);
}

// ============================================================
//  Renderer2D
// ============================================================
Renderer2D::~Renderer2D() { shutdown(); }

bool Renderer2D::init(i32 fbWidth, i32 fbHeight) noexcept {
    if (m_inited) return true;

#ifdef POCKET_GLES3
    constexpr const char* kGlName = "GLES3";
#else
    constexpr const char* kGlName = "GL";
#endif
    PK_LOG_INFO("Renderer", "Initializing Renderer2D (GL=%s)", kGlName);

    PK_LOG_INFO("Renderer", "GL_VERSION: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    PK_LOG_INFO("Renderer", "GL_RENDERER: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    PK_LOG_INFO("Renderer", "GL_VENDOR: %s", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

    initShaders();
    initBuffers();

    // Init texture slots: slot 0 = white (solid color)
    m_textureSlots[0] = 0;  // we'll bind a 1x1 white texture here
    u8 white[4] = {255,255,255,255};
    m_textureSlots[0] = uploadTexture(white, 1, 1, 4, true, false);
    m_textureSlotCount = 1;

    setViewport(0, 0, fbWidth, fbHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_inited = true;
    PK_LOG_INFO("Renderer", "Renderer2D ready (max quads=%u, max textures=%u)",
                MAX_BATCH_QUADS, MAX_TEXTURE_SLOTS);
    return true;
}

void Renderer2D::shutdown() noexcept {
    if (!m_inited) return;
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
    if (m_lineVAO) glDeleteVertexArrays(1, &m_lineVAO);
    if (m_lineVBO) glDeleteBuffers(1, &m_lineVBO);
    if (m_batchShader.program) glDeleteProgram(m_batchShader.program);
    if (m_lineShader.program)  glDeleteProgram(m_lineShader.program);
    m_inited = false;
}

void Renderer2D::initShaders() noexcept {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   kBatchVertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kBatchFragSrc);
    m_batchShader.program = linkProgram(vs, fs);
    m_batchShader.name    = "Batch2D";

    GLuint lvs = compileShader(GL_VERTEX_SHADER,   kLineVertSrc);
    GLuint lfs = compileShader(GL_FRAGMENT_SHADER, kLineFragSrc);
    m_lineShader.program = linkProgram(lvs, lfs);
    m_lineShader.name    = "Line2D";
}

void Renderer2D::initBuffers() noexcept {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), nullptr, GL_DYNAMIC_DRAW);

    // Indices (static - never change)
    Vec<u32> indices(MAX_INDICES);
    u32 offset = 0;
    for (u32 i = 0; i < MAX_INDICES; i += 6) {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
        offset += 4;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * indices.size(),
                 indices.data(), GL_STATIC_DRAW);

    // Vertex attribs
    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(Vertex2D));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex2D, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex2D, color)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex2D, uv)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex2D, texIndex)));

    glBindVertexArray(0);

    // Line buffers
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_lineVertices), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex2D, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex2D, color)));
    glBindVertexArray(0);
}

void Renderer2D::setViewport(i32 x, i32 y, i32 w, i32 h) noexcept {
    glViewport(x, y, w, h);
}
void Renderer2D::setClearColor(f32 r, f32 g, f32 b, f32 a) noexcept {
    glClearColor(r, g, b, a);
}
void Renderer2D::clear() noexcept { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void Renderer2D::beginScene(const math::Mat4& vp) noexcept {
    m_viewProj = vp;
    m_vertexCount = 0;
    m_indexCount = 0;
    m_lineVertexCount = 0;
    m_textureSlotCount = 1;  // slot 0 reserved for white
    resetStats();
}

void Renderer2D::endScene() noexcept {
    flush();
    // Lines
    if (m_lineVertexCount > 0) {
        m_lineShader.use();
        m_lineShader.setMat4("uViewProj", m_viewProj);
        glBindVertexArray(m_lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_lineVertexCount * sizeof(Vertex2D), m_lineVertices);
        glDrawArrays(GL_LINES, 0, m_lineVertexCount);
        glBindVertexArray(0);
        m_stats.drawCalls++;
        m_stats.vertices += m_lineVertexCount;
    }
}

void Renderer2D::flush() noexcept {
    if (m_vertexCount == 0) return;

    m_batchShader.use();
    m_batchShader.setMat4("uViewProj", m_viewProj);

    // Bind textures
    i32 slots[MAX_TEXTURE_SLOTS];
    for (u32 i = 0; i < MAX_TEXTURE_SLOTS; ++i) slots[i] = static_cast<i32>(i);
    GLint loc = glGetUniformLocation(m_batchShader.program, "uTextures");
    if (loc >= 0) glUniform1iv(loc, MAX_TEXTURE_SLOTS, slots);

    for (u32 i = 0; i < m_textureSlotCount; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureSlots[i]);
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexCount * sizeof(Vertex2D), m_vertices);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    m_stats.drawCalls++;
    m_stats.vertices += m_vertexCount;
    m_stats.quads    += m_indexCount / 6;

    m_vertexCount = 0;
    m_indexCount = 0;
    m_textureSlotCount = 1;  // reset to just white
}

void Renderer2D::drawQuad(f32 x, f32 y, f32 w, f32 h, const math::Vec4& color) noexcept {
    drawSprite(0, x, y, w, h, 0, 0, 1, 1, color);
}

void Renderer2D::drawSprite(u32 textureID, f32 x, f32 y, f32 w, f32 h,
                             f32 u0, f32 v0, f32 u1, f32 v1,
                             const math::Vec4& color) noexcept {
    // Find or assign texture slot
    f32 texIndex = 0.0f;
    if (textureID != 0) {
        bool found = false;
        for (u32 i = 1; i < m_textureSlotCount; ++i) {
            if (m_textureSlots[i] == textureID) {
                texIndex = static_cast<f32>(i);
                found = true; break;
            }
        }
        if (!found) {
            if (m_textureSlotCount >= MAX_TEXTURE_SLOTS) flush();
            if (m_vertexCount + 4 > MAX_VERTICES)        flush();
            m_textureSlots[m_textureSlotCount] = textureID;
            texIndex = static_cast<f32>(m_textureSlotCount);
            m_textureSlotCount++;
        }
    }

    if (m_vertexCount + 4 > MAX_VERTICES || m_indexCount + 6 > MAX_INDICES) flush();

    Vertex2D* v = &m_vertices[m_vertexCount];
    v[0] = { {x,     y,     0}, color, {u0, v0}, texIndex };
    v[1] = { {x + w, y,     0}, color, {u1, v0}, texIndex };
    v[2] = { {x + w, y + h, 0}, color, {u1, v1}, texIndex };
    v[3] = { {x,     y + h, 0}, color, {u0, v1}, texIndex };
    m_vertexCount += 4;
    m_indexCount  += 6;
}

void Renderer2D::drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const math::Vec4& color) noexcept {
    if (m_lineVertexCount + 2 > MAX_LINE_VERTICES) {
        // Flush lines immediately by drawing now
        endScene();
    }
    Vertex2D* v = &m_lineVertices[m_lineVertexCount];
    v[0] = { {x0, y0, 0}, color, {0,0}, 0 };
    v[1] = { {x1, y1, 0}, color, {0,0}, 0 };
    m_lineVertexCount += 2;
}

void Renderer2D::drawCircle(f32 cx, f32 cy, f32 r, const math::Vec4& color, u32 segs) noexcept {
    if (segs < 3) segs = 3;
    f32 step = 2.0f * 3.14159265f / segs;
    for (u32 i = 0; i < segs; ++i) {
        f32 a0 = i * step, a1 = (i + 1) * step;
        drawLine(cx + std::cos(a0) * r, cy + std::sin(a0) * r,
                 cx + std::cos(a1) * r, cy + std::sin(a1) * r, color);
    }
}

u32 Renderer2D::uploadTexture(const u8* pixels, i32 w, i32 h, i32 channels,
                               bool smooth, bool repeat) noexcept {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    GLenum fmt = GL_RGBA;
    if (channels == 1) fmt = GL_RED;
    else if (channels == 3) fmt = GL_RGB;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_stats.textures++;
    return id;
}

void Renderer2D::deleteTexture(u32 id) noexcept {
    if (id) glDeleteTextures(1, &id);
}

Renderer2D& renderer() noexcept {
    static Renderer2D r;
    return r;
}

} // namespace pk::rnd
