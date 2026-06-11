// =============================================================================
//  draw2D.h  -  Personal 2D drawing helper library for OpenGL 3.3+
//  Author : Ivan Sardon Medina
//
//  Lighting support is separated into lighting.h and included before Shape.
//
//  Transforms: see transform_math.h (Mat4, Transform, trs2D, PI).  Vertex shader:
//  gl_Position = uModel * vec4(aPos, 1.0).
//
//  ---------------------------------------------------------------------------
//  Quick start
//  ---------------------------------------------------------------------------
//      #include "gl2d.h"
//
//      // Create shapes (after a valid GL context exists):
//      gl2d::Circle    pepperoni(0.1f);
//      pepperoni.color = {0.74f, 0.13f, 0.02f, 1.0f};
//      pepperoni.transform.matrix = gl2d::Mat4::translate2D(0.3f, 0.0f);
//
//      gl2d::Rectangle box(0.4f, 0.2f);
//      box.color = gl2d::colors::Cyan;
//
//      // Arrange a bunch of shapes in a ring:
//      gl2d::ShapeGroup ring;
//      for (int i = 0; i < 6; ++i)
//          ring.addOwned(make_unique<gl2d::Circle>(0.08f));
//      ring.arrangeCircular(0.5f);
//      for (size_t i = 0; i < ring.size(); ++i)
//          ring[i]->color = gl2d::colors::Red;
//
//      // In the render loop:
//      pepperoni.draw();
//      box.draw();
//      ring.draw();
//
//  By default every shape uses the library's built-in shader.  If you want to
//  plug your own program in, either:
//      1) build a gl2d::Shader from your own vertex/fragment sources:
//             gl2d::Shader mine(myVS, myFS, {"uModel","uColor"});
//             circle.shader = &mine;
//      2) or adopt an already-linked program:
//             auto s = gl2d::Shader::adopt(myProgram, {"uM","uC"});
//             circle.shader = &s;
//
//  The default shader expects:
//      layout(location = 0) in vec3 aPos;
//      uniform mat4 uModel;   // column-major, same layout as glUniformMatrix4fv
//      uniform vec4 uColor;
//
//  Per-vertex color (uploadWithColor): same as above plus
//      layout(location = 1) in vec4 aColor;
//      fragment color = uColor * aColor (tint × per-vertex)
// =============================================================================

#ifndef GL2D_H_INCLUDED
#define GL2D_H_INCLUDED

#include "transformations.h"

#include <glad/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#ifndef GL2D_NO_STB_IMAGE
#ifndef GL2D_STB_IMAGE_INCLUDED
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include <stb_image.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#define GL2D_STB_IMAGE_INCLUDED
#endif
#endif

using namespace std;

// -----------------------------------------------------------------------------
//  Color
// -----------------------------------------------------------------------------
class Color {
public:
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

namespace colors {
    constexpr Color White   {1.0f, 1.0f, 1.0f, 1.0f};
    constexpr Color Black   {0.0f, 0.0f, 0.0f, 1.0f};
    constexpr Color Red     {1.0f, 0.0f, 0.0f, 1.0f};
    constexpr Color Green   {0.0f, 1.0f, 0.0f, 1.0f};
    constexpr Color Blue    {0.0f, 0.0f, 1.0f, 1.0f};
    constexpr Color Yellow  {1.0f, 1.0f, 0.0f, 1.0f};
    constexpr Color Magenta {1.0f, 0.0f, 1.0f, 1.0f};
    constexpr Color Cyan    {0.0f, 1.0f, 1.0f, 1.0f};
    constexpr Color Orange  {1.0f, 0.5f, 0.0f, 1.0f};
    constexpr Color Gray    {0.5f, 0.5f, 0.5f, 1.0f};
}

// -----------------------------------------------------------------------------
//  DrawPrim
//  Choose rasterization style when constructing shapes.
// -----------------------------------------------------------------------------
enum class DrawPrim {
    Filled,    // solid fill (triangles / fans / indexed tris as appropriate)
    Lines,     // edges / outline / wireframe
    Vertices   // corner / perimeter samples as GL_POINTS
};

// -----------------------------------------------------------------------------
//  Textures and practical effects
// -----------------------------------------------------------------------------
class TextureOptions {
public:
    bool flipVertically = true;
    bool srgb = false;
    bool generateMipmaps = true;
    GLenum wrapS = GL_REPEAT;
    GLenum wrapT = GL_REPEAT;
    GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;
    GLenum magFilter = GL_LINEAR;
};

class Texture2D {
public:
    GLuint id = 0;
    int width = 0;
    int height = 0;
    int channels = 0;

    bool valid() const { return id != 0; }
};

inline void enableAlphaBlending() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

inline void enableAdditiveBlending() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

inline void disableBlending() {
    glDisable(GL_BLEND);
}

inline Texture2D loadTexture2D(const string& path,
                               const TextureOptions& options = {}) {
    Texture2D texture{};
#ifdef GL2D_NO_STB_IMAGE
    (void)path;
    (void)options;
    fprintf(stderr, "gl2d: loadTexture2D requires stb_image; remove GL2D_NO_STB_IMAGE\n");
    return texture;
#else
    stbi_set_flip_vertically_on_load(options.flipVertically ? 1 : 0);
    unsigned char* data = stbi_load(path.c_str(), &texture.width, &texture.height,
                                    &texture.channels, 0);
    if (!data) {
        fprintf(stderr, "gl2d: failed to load texture '%s': %s\n",
                path.c_str(), stbi_failure_reason());
        return texture;
    }

    GLenum srcFormat = GL_RGB;
    GLenum internalFormat = GL_RGB;
    if (texture.channels == 1) {
        srcFormat = GL_RED;
        internalFormat = GL_RED;
    } else if (texture.channels == 3) {
        srcFormat = GL_RGB;
        internalFormat = options.srgb ? GL_SRGB : GL_RGB;
    } else if (texture.channels == 4) {
        srcFormat = GL_RGBA;
        internalFormat = options.srgb ? GL_SRGB_ALPHA : GL_RGBA;
    }

    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
                 texture.width, texture.height, 0, srcFormat,
                 GL_UNSIGNED_BYTE, data);
    if (options.generateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(options.wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(options.wrapT));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(options.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(options.magFilter));
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return texture;
#endif
}

inline void deleteTexture(Texture2D& texture) {
    if (texture.id != 0) {
        glDeleteTextures(1, &texture.id);
        texture.id = 0;
    }
    texture.width = texture.height = texture.channels = 0;
}

// -----------------------------------------------------------------------------
//  Shader
//  Uniform names default to those expected by the built-in shader, but can be
//  overridden so you can plug in any program of your own.
// -----------------------------------------------------------------------------
class UniformNames {
public:
    string projection = "uProjection";
    string view = "uView";
    string model = "uModel";
    string color = "uColor";
};

class Shader {
public:
    Shader() = default;

    Shader(const char* vertSrc, const char* fragSrc, UniformNames names = {}) {
        buildFromSource(vertSrc, fragSrc, names);
    }

    // Wrap an existing linked program without taking ownership of it.
    static Shader adopt(GLuint program, UniformNames names = {}) {
        Shader s;
        s.program_ = program;
        s.owns_    = false;
        s.cacheLocations(names);
        return s;
    }

    ~Shader() {
        if (owns_ && program_ != 0) {
            glDeleteProgram(program_);
        }
    }

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& o) noexcept { moveFrom(::move(o)); }
    Shader& operator=(Shader&& o) noexcept {
        if (this != &o) {
            if (owns_ && program_) glDeleteProgram(program_);
            moveFrom(::move(o));
        }
        return *this;
    }

    GLuint program()  const { return program_; }
    GLint  locProjection() const { return locProjection_; }
    GLint  locView() const { return locView_; }
    GLint  locModel() const { return locModel_; }
    GLint  locColor() const { return locColor_; }

    void use() const { glUseProgram(program_); }

private:
    void buildFromSource(const char* vs, const char* fs, const UniformNames& names) {
        const GLuint v = compile(GL_VERTEX_SHADER,   vs);
        const GLuint f = compile(GL_FRAGMENT_SHADER, fs);
        program_ = glCreateProgram();
        glAttachShader(program_, v);
        glAttachShader(program_, f);
        glLinkProgram(program_);

        GLint ok = 0;
        glGetProgramiv(program_, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024] = {0};
            glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
            fprintf(stderr, "gl2d: program link error: %s\n", log);
        }
        glDeleteShader(v);
        glDeleteShader(f);

        owns_ = true;
        cacheLocations(names);
    }

    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024] = {0};
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            fprintf(stderr, "gl2d: shader compile error: %s\n", log);
        }
        return s;
    }

    void cacheLocations(const UniformNames& n) {
        locProjection_ = glGetUniformLocation(program_, n.projection.c_str());
        locView_ = glGetUniformLocation(program_, n.view.c_str());
        locModel_ = glGetUniformLocation(program_, n.model.c_str());
        locColor_ = glGetUniformLocation(program_, n.color.c_str());
    }

    void moveFrom(Shader&& o) {
        program_   = o.program_;
        owns_      = o.owns_;
        locProjection_ = o.locProjection_;
        locView_ = o.locView_;
        locModel_  = o.locModel_;
        locColor_  = o.locColor_;
        o.program_ = 0;
        o.owns_    = false;
    }

    GLuint program_  = 0;
    bool   owns_     = false;
    GLint  locProjection_ = -1;
    GLint  locView_ = -1;
    GLint  locModel_ = -1;
    GLint  locColor_ = -1;
};

inline Mat4& globalProjectionMatrix() {
    static Mat4 proj = Mat4::identity();
    return proj;
}

inline Mat4& globalViewMatrix() {
    static Mat4 view = Mat4::identity();
    return view;
}

inline void setProjectionMatrix(const Mat4& projection) { globalProjectionMatrix() = projection; }
inline void setViewMatrix(const Mat4& view) { globalViewMatrix() = view; }

// Built-in default shader. Lazy-compiled on first use: a valid GL context must
// exist before the first shape draws.
inline Shader& defaultShader() {
    static const char* kVert =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "uniform mat4 uProjection;\n"
        "uniform mat4 uView;\n"
        "uniform mat4 uModel;\n"
        "void main() {\n"
        "    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);\n"
        "}\n";

    static const char* kFrag =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec4 uColor;\n"
        "void main() { FragColor = uColor; }\n";

    static Shader s(kVert, kFrag);
    return s;
}

// Default shader for meshes uploaded with uploadWithColor (pos + rgba per vertex).
inline Shader& defaultShaderVertexColor() {
    static const char* kVert =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec4 aColor;\n"
        "uniform mat4 uProjection;\n"
        "uniform mat4 uView;\n"
        "uniform mat4 uModel;\n"
        "out vec4 vColor;\n"
        "void main() {\n"
        "    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);\n"
        "    vColor = aColor;\n"
        "}\n";

    static const char* kFrag =
        "#version 330 core\n"
        "in vec4 vColor;\n"
        "out vec4 FragColor;\n"
        "uniform vec4 uColor;\n"
        "void main() { FragColor = uColor * vColor; }\n";

    static Shader s(kVert, kFrag);
    return s;
}

inline void setUniformMat4(const Shader& shader, const char* name, const Mat4& value) {
    const GLint loc = glGetUniformLocation(shader.program(), name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
}

inline void setUniformColor(const Shader& shader, const char* name, const Color& value) {
    const GLint loc = glGetUniformLocation(shader.program(), name);
    if (loc >= 0) glUniform4f(loc, value.r, value.g, value.b, value.a);
}

inline void setUniform1f(const Shader& shader, const char* name, float value) {
    const GLint loc = glGetUniformLocation(shader.program(), name);
    if (loc >= 0) glUniform1f(loc, value);
}

inline void setUniform1i(const Shader& shader, const char* name, int value) {
    const GLint loc = glGetUniformLocation(shader.program(), name);
    if (loc >= 0) glUniform1i(loc, value);
}

#include "lighting.h"

// -----------------------------------------------------------------------------
//  Shape  (base class for every drawable primitive)
// -----------------------------------------------------------------------------
class Shape {
public:
    Color     color     = colors::White;
    Transform transform = {};
    Shader*   shader    = nullptr;                 // nullptr -> defaultShader()
    GLenum    primitive = GL_TRIANGLE_FAN;         // set by each subclass
    bool      lightingEnabled = false;
    Material  material = globalDefaultMaterial();

    // Optional per-shape line width / point size. Only applied when > 0.
    float lineWidth = 3.0f;
    float pointSize = 5.0f;

    Shape()          = default;
    virtual ~Shape() { cleanup(); }

    Shape(const Shape&)            = delete;
    Shape& operator=(const Shape&) = delete;

    Shape(Shape&& o) noexcept { moveFrom(::move(o)); }
    Shape& operator=(Shape&& o) noexcept {
        if (this != &o) { cleanup(); moveFrom(::move(o)); }
        return *this;
    }

    // Draws the shape using its own color and transform matrix.
    void draw() const { draw(color, transform.matrix); }

    void enableLighting(const Material& materialIn) {
        material = materialIn;
        lightingEnabled = true;
    }

    void enableLighting() {
        lightingEnabled = true;
    }

    void disableLighting() {
        lightingEnabled = false;
    }

    bool hasLightingData() const {
        return hasNormals_;
    }

    // Draws with an explicit model matrix (column-major). ShapeGroup passes the
    // product (groupMatrix * childMatrix).
    void draw(const Color& c, const Mat4& model) const {
        if (vao_ == 0) return;
        const bool useBuiltInLighting = shader == nullptr && lightingEnabled && hasNormals_;
        Shader& sh = shader ? *shader
                             : (useBuiltInLighting ? defaultShaderLitBlinnPhong()
                                                   : (perVertexColor_ ? defaultShaderVertexColor()
                                                                      : defaultShader()));
        sh.use();
        if (useBuiltInLighting) {
            uploadLightingUniforms(sh, c, material, model, hasTexCoords_);
        } else {
            if (sh.locProjection() >= 0) glUniformMatrix4fv(sh.locProjection(), 1, GL_FALSE, globalProjectionMatrix().data());
            if (sh.locView() >= 0) glUniformMatrix4fv(sh.locView(), 1, GL_FALSE, globalViewMatrix().data());
            if (sh.locModel() >= 0) glUniformMatrix4fv(sh.locModel(), 1, GL_FALSE, model.data());
            if (sh.locColor() >= 0) glUniform4f(sh.locColor(), c.r, c.g, c.b, c.a);
        }

        if (lineWidth > 0.0f) glLineWidth(lineWidth);
        if (pointSize > 0.0f) glPointSize(pointSize);

        glBindVertexArray(vao_);
        if (indexCount_ > 0) {
            glDrawElements(primitive, indexCount_, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(primitive, 0, vertexCount_);
        }
        glBindVertexArray(0);
    }

protected:
    // Upload interleaved xyz + rgba (7 floats per vertex). Fragment = uColor * aColor.
    void uploadWithColor(const vector<float>& interleavedPosColor,
                         const vector<unsigned int>& indices = {}) {
        cleanup();
        vertexCount_ = static_cast<GLsizei>(interleavedPosColor.size() / 7);
        indexCount_  = static_cast<GLsizei>(indices.size());
        if (vertexCount_ == 0) return;
        perVertexColor_ = true;
        hasNormals_ = false;
        hasTexCoords_ = false;

        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(interleavedPosColor.size() * sizeof(float)),
                     interleavedPosColor.data(),
                     GL_STATIC_DRAW);
        constexpr GLsizei stride = 7 * static_cast<GLsizei>(sizeof(float));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        if (!indices.empty()) {
            glGenBuffers(1, &ebo_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                         indices.data(),
                         GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
    }

    // Upload interleaved xyz vertex data (3 floats per vertex).
    // Call from a subclass constructor. If 'indices' is empty, glDrawArrays
    // will be used; otherwise glDrawElements.
    void upload(const vector<float>& verts,
                const vector<unsigned int>& indices = {}) {
        cleanup();
        vertexCount_ = static_cast<GLsizei>(verts.size() / 3);
        indexCount_  = static_cast<GLsizei>(indices.size());
        if (vertexCount_ == 0) return;
        perVertexColor_ = false;
        hasNormals_ = false;
        hasTexCoords_ = false;

        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        if (!indices.empty()) {
            glGenBuffers(1, &ebo_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                         indices.data(),
                         GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
    }

    // Upload interleaved xyz + rgba + normal + uv (12 floats per vertex).
    // Lit meshes still render with the old vertex-color shader until lighting is enabled.
    void uploadLit(const vector<float>& interleaved,
                   const vector<unsigned int>& indices = {},
                   bool hasTexCoords = true) {
        cleanup();
        vertexCount_ = static_cast<GLsizei>(interleaved.size() / 12);
        indexCount_  = static_cast<GLsizei>(indices.size());
        if (vertexCount_ == 0) return;
        perVertexColor_ = true;
        hasNormals_ = true;
        hasTexCoords_ = hasTexCoords;

        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(interleaved.size() * sizeof(float)),
                     interleaved.data(),
                     GL_STATIC_DRAW);

        constexpr GLsizei stride = 12 * static_cast<GLsizei>(sizeof(float));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(10 * sizeof(float)));
        glEnableVertexAttribArray(3);

        if (!indices.empty()) {
            glGenBuffers(1, &ebo_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                         indices.data(),
                         GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
    }

    GLuint  vao_         = 0;
    GLuint  vbo_         = 0;
    GLuint  ebo_         = 0;
    GLsizei vertexCount_ = 0;
    GLsizei indexCount_  = 0;
    bool    perVertexColor_ = false;
    bool    hasNormals_ = false;
    bool    hasTexCoords_ = false;

private:
    void cleanup() {
        if (ebo_) { glDeleteBuffers(1, &ebo_);      ebo_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_);      vbo_ = 0; }
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        vertexCount_ = 0;
        indexCount_  = 0;
        perVertexColor_ = false;
        hasNormals_ = false;
        hasTexCoords_ = false;
    }

    void moveFrom(Shape&& o) {
        color        = o.color;
        transform    = o.transform;
        shader       = o.shader;
        primitive    = o.primitive;
        lightingEnabled = o.lightingEnabled;
        material     = o.material;
        lineWidth    = o.lineWidth;
        pointSize    = o.pointSize;
        vao_         = o.vao_;
        vbo_         = o.vbo_;
        ebo_         = o.ebo_;
        vertexCount_ = o.vertexCount_;
        indexCount_  = o.indexCount_;
        perVertexColor_ = o.perVertexColor_;
        hasNormals_ = o.hasNormals_;
        hasTexCoords_ = o.hasTexCoords_;
        o.vao_ = o.vbo_ = o.ebo_ = 0;
        o.vertexCount_ = 0;
        o.indexCount_  = 0;
        o.perVertexColor_ = false;
        o.hasNormals_ = false;
        o.hasTexCoords_ = false;
        o.lightingEnabled = false;
    }
};

// -----------------------------------------------------------------------------
//  Primitives
// -----------------------------------------------------------------------------

// Filled disc (TRIANGLE_FAN) or outline ring (LINE_LOOP).
class Circle : public Shape {
public:
    Circle(float radius, int segments = 64,
           DrawPrim prim = DrawPrim::Filled) {
        if (segments < 3) segments = 3;
        vector<float> verts;
        if (prim == DrawPrim::Filled) {
            verts.reserve(static_cast<size_t>(segments + 2) * 3);
            verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
            for (int i = 0; i <= segments; ++i) {
                const float a = 2.0f * PI * float(i) / float(segments);
                verts.push_back(radius * cos(a));
                verts.push_back(radius * sin(a));
                verts.push_back(0.0f);
            }
            primitive = GL_TRIANGLE_FAN;
        } else if (prim == DrawPrim::Lines) {
            verts.reserve(static_cast<size_t>(segments) * 3);
            for (int i = 0; i < segments; ++i) {
                const float a = 2.0f * PI * float(i) / float(segments);
                verts.push_back(radius * cos(a));
                verts.push_back(radius * sin(a));
                verts.push_back(0.0f);
            }
            primitive = GL_LINE_LOOP;
        } else {
            verts.reserve(static_cast<size_t>(segments) * 3);
            for (int i = 0; i < segments; ++i) {
                const float a = 2.0f * PI * float(i) / float(segments);
                verts.push_back(radius * cos(a));
                verts.push_back(radius * sin(a));
                verts.push_back(0.0f);
            }
            primitive = GL_POINTS;
            pointSize = 2.0f;
        }
        upload(verts);
    }
};

// Axis-aligned rectangle centered at origin.
class Rectangle : public Shape {
public:
    Rectangle(float width, float height,
              DrawPrim prim = DrawPrim::Filled) {
        const float hw = width  * 0.5f;
        const float hh = height * 0.5f;
        vector<float> verts = {
            -hw, -hh, 0.0f,
             hw, -hh, 0.0f,
             hw,  hh, 0.0f,
            -hw,  hh, 0.0f
        };
        if (prim == DrawPrim::Filled) {
            vector<unsigned int> idx = { 0, 1, 2,  0, 2, 3 };
            primitive = GL_TRIANGLES;
            upload(verts, idx);
        } else if (prim == DrawPrim::Lines) {
            primitive = GL_LINE_LOOP;
            upload(verts);
        } else {
            primitive = GL_POINTS;
            pointSize = 5.0f;
            upload(verts);
        }
    }
};

class Trapecio : public Shape {
public:
    Trapecio(float width, float height, float inclin,
              DrawPrim prim = DrawPrim::Filled) {
        const float hw = width  * 0.5f;
        const float hh = height * 0.5f;
        vector<float> verts = {
            -hw, -hh, 0.0f,
             hw, -hh, 0.0f,
             hw-inclin,  hh, 0.0f,
            -hw+inclin,  hh, 0.0f
        };
        if (prim == DrawPrim::Filled) {
            vector<unsigned int> idx = { 0, 1, 2,  0, 2, 3 };
            primitive = GL_TRIANGLES;
            upload(verts, idx);
        } else if (prim == DrawPrim::Lines) {
            primitive = GL_LINE_LOOP;
            upload(verts);
        } else {
            primitive = GL_POINTS;
            pointSize = 5.0f;
            upload(verts);
        }
    }
};

// Triangle from three free 2D vertices.
class Triangle : public Shape {
public:
    Triangle(float x0, float y0,
             float x1, float y1,
             float x2, float y2,
             DrawPrim prim = DrawPrim::Filled) {
        vector<float> verts = {
            x0, y0, 0.0f,
            x1, y1, 0.0f,
            x2, y2, 0.0f
        };
        if (prim == DrawPrim::Filled) primitive = GL_TRIANGLES;
        else if (prim == DrawPrim::Lines) primitive = GL_LINE_LOOP;
        else {
            primitive = GL_POINTS;
            pointSize = 2.0f;
        }
        upload(verts);
    }
};

// Regular N-sided polygon inscribed in a circle of the given radius.
class RegularPolygon : public Shape {
public:
    RegularPolygon(float radius, int sides,
                   DrawPrim prim = DrawPrim::Filled) {
        if (sides < 3) sides = 3;
        vector<float> verts;
        if (prim == DrawPrim::Filled) {
            verts.reserve(static_cast<size_t>(sides + 2) * 3);
            verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
            for (int i = 0; i <= sides; ++i) {
                const float a = 2.0f * PI * float(i) / float(sides);
                verts.push_back(radius * cos(a));
                verts.push_back(radius * sin(a));
                verts.push_back(0.0f);
            }
            primitive = GL_TRIANGLE_FAN;
        } else if (prim == DrawPrim::Lines) {
            verts.reserve(static_cast<size_t>(sides) * 3);
            for (int i = 0; i < sides; ++i) {
                const float a = 2.0f * PI * float(i) / float(sides);
                verts.push_back(radius * cos(a));
                verts.push_back(radius * sin(a));
                verts.push_back(0.0f);
            }
            primitive = GL_LINE_LOOP;
        } else {
            verts.reserve(static_cast<size_t>(sides) * 3);
            for (int i = 0; i < sides; ++i) {
                const float a = 2.0f * PI * float(i) / float(sides);
                verts.push_back(radius * cos(a));
                verts.push_back(radius * sin(a));
                verts.push_back(0.0f);
            }
            primitive = GL_POINTS;
            pointSize = 2.0f;
        }
        upload(verts);
    }
};

// Circular sector / pie slice from angle0 to angle1 (radians).
class Wedge : public Shape {
public:
    Wedge(float radius, float angle0, float angle1, int arcSegments = 32,
          DrawPrim prim = DrawPrim::Filled) {
        if (arcSegments < 2) arcSegments = 2;
        vector<float> verts;
        verts.reserve(static_cast<size_t>(arcSegments + 2) * 3);
        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
        for (int k = 0; k <= arcSegments; ++k) {
            const float t = float(k) / float(arcSegments);
            const float a = angle0 + t * (angle1 - angle0);
            verts.push_back(radius * cos(a));
            verts.push_back(radius * sin(a));
            verts.push_back(0.0f);
        }
        if (prim == DrawPrim::Filled) {
            primitive = GL_TRIANGLE_FAN;
        } else if (prim == DrawPrim::Lines) {
            // Center -> start rim -> arc -> end rim -> center (outline).
            verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);
            primitive = GL_LINE_STRIP;
        } else {
            primitive = GL_POINTS;
            pointSize = 2.0f;
        }
        upload(verts);
    }
};

// Rectangular strip anchored between two radii and rotated around the origin.
// Useful for radial decorations such as the "bacon" strips in the pizza.
class RingStrip : public Shape {
public:
    RingStrip(float rInner, float rOuter, float width, float angleRad,
              DrawPrim prim = DrawPrim::Filled) {
        const float w2   = width * 0.5f;
        const float hOut = sqrt(max(0.0f, rOuter * rOuter - w2 * w2));
        const float hIn  = sqrt(max(0.0f, rInner * rInner - w2 * w2));

        const float lx[4] = { -w2,  w2,  w2, -w2 };
        const float ly[4] = { hOut, hOut, hIn, hIn };

        const float c = cos(angleRad);
        const float s = sin(angleRad);

        vector<float> verts;
        verts.reserve(12);
        for (int i = 0; i < 4; ++i) {
            const float wx = lx[i] * c - ly[i] * s;
            const float wy = lx[i] * s + ly[i] * c;
            verts.push_back(wx);
            verts.push_back(wy);
            verts.push_back(0.0f);
        }
        if (prim == DrawPrim::Filled) {
            vector<unsigned int> idx = { 0, 1, 2,  0, 2, 3 };
            primitive = GL_TRIANGLES;
            upload(verts, idx);
        } else if (prim == DrawPrim::Lines) {
            primitive = GL_LINE_LOOP;
            upload(verts);
        } else {
            primitive = GL_POINTS;
            pointSize = 2.0f;
            upload(verts);
        }
    }
};

// Point cloud. Useful for "oregano"-style scatter effects.
class Points : public Shape {
public:
    Points(const vector<pair<float, float>>& pts, float pointSz = 2.0f) {
        vector<float> verts;
        verts.reserve(pts.size() * 3);
        for (const auto& p : pts) {
            verts.push_back(p.first);
            verts.push_back(p.second);
            verts.push_back(0.0f);
        }
        pointSize = pointSz;
        primitive = GL_POINTS;
        upload(verts);
    }

    // Helper: generate 'count' points uniformly distributed inside a disc.
    static Points randomInDisc(float radius, int count,
                               float pointSz = 2.0f, uint32_t seed = 0) {
        mt19937 rng(seed ? seed : random_device{}());
        uniform_real_distribution<float> u(0.0f, 1.0f);
        vector<pair<float, float>> pts;
        pts.reserve(static_cast<size_t>(max(0, count)));
        for (int i = 0; i < count; ++i) {
            const float th = 2.0f * PI * u(rng);
            const float r  = radius * sqrt(u(rng));
            pts.emplace_back(r * cos(th), r * sin(th));
        }
        return Points(pts, pointSz);
    }

    // Helper: generate 'count' points inside a circular sector.
    static Points randomInWedge(float radius, float angle0, float angle1,
                                int count, float pointSz = 2.0f,
                                uint32_t seed = 0) {
        mt19937 rng(seed ? seed : random_device{}());
        uniform_real_distribution<float> u(0.0f, 1.0f);
        vector<pair<float, float>> pts;
        pts.reserve(static_cast<size_t>(max(0, count)));
        for (int i = 0; i < count; ++i) {
            const float th = angle0 + u(rng) * (angle1 - angle0);
            const float r  = radius * sqrt(u(rng));
            pts.emplace_back(r * cos(th), r * sin(th));
        }
        return Points(pts, pointSz);
    }
};

// Single straight line segment.
class Line : public Shape {
public:
    Line(float x0, float y0, float x1, float y1, float width = 1.0f,
         DrawPrim prim = DrawPrim::Lines) {
        vector<float> verts = {
            x0, y0, 0.0f,
            x1, y1, 0.0f
        };
        lineWidth = width;
        if (prim == DrawPrim::Vertices) {
            primitive = GL_POINTS;
            pointSize = max(width, 1.0f);
        } else {
            primitive = GL_LINES;
        }
        upload(verts);
    }
};

// -----------------------------------------------------------------------------
//  ShapeGroup
//  Collection of shapes and/or nested groups with its own transform and layout
//  helpers. Child order is preserved: add() / addGroup() order matches layout.
//  World matrix: parentWorld * this->transform * childLocal * shapeLocal.
// -----------------------------------------------------------------------------
class ShapeGroup {
public:
    Transform transform = {};

    // Add a non-owned shape (caller keeps ownership).
    void add(Shape* s) {
        if (s) children_.push_back({s, nullptr});
    }

    // Add a non-owned child group (caller keeps ownership).
    void addGroup(ShapeGroup* g) {
        if (g) children_.push_back({nullptr, g});
    }

    // Add a shape whose lifetime is managed by this group.
    void addOwned(unique_ptr<Shape> s) {
        if (!s) return;
        Shape* p = s.get();
        owned_.push_back(::move(s));
        children_.push_back({p, nullptr});
    }

    // Remove a child from this group without calling delete.
    // If the shape was added with addOwned(), ownership stays in this group.
    bool remove(Shape* s) {
        if (!s) return false;
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (it->shape == s) {
                children_.erase(it);
                return true;
            }
        }
        return false;
    }

    // Remove a child group from this group without calling delete.
    bool removeGroup(ShapeGroup* g) {
        if (!g) return false;
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (it->group == g) {
                children_.erase(it);
                return true;
            }
        }
        return false;
    }

    void clear() {
        children_.clear();
        owned_.clear();
    }

    size_t size() const { return children_.size(); }
    bool   empty() const { return children_.empty(); }

    // Shape at index, or nullptr if that slot is a nested group.
    Shape* operator[](size_t i) const {
        if (i >= children_.size()) return nullptr;
        return children_[i].shape;
    }

    ShapeGroup* childGroupAt(size_t i) const {
        if (i >= children_.size()) return nullptr;
        return children_[i].group;
    }

    // Draw all descendants. Optional parentWorld for nested use (default: identity).
    void draw() const { draw(Mat4::identity()); }

    void draw(const Mat4& parentWorld) const {
        const Mat4 world = parentWorld * transform.matrix;
        for (const Child& ch : children_) {
            if (ch.shape) {
                const Mat4 w = world * ch.shape->transform.matrix;
                ch.shape->draw(ch.shape->color, w);
            } else if (ch.group) {
                ch.group->draw(world);
            }
        }
    }

    // Tint every shape in this subtree.
    void setColor(const Color& c) {
        for (const Child& ch : children_) {
            if (ch.shape) ch.shape->color = c;
            else if (ch.group) ch.group->setColor(c);
        }
    }

    // -------- Layout helpers (shapes and child groups in insertion order) --------

    void arrangeCircular(float radius, float startAngle = 0.0f, bool faceOutward = false) {
        if (children_.empty()) return;
        const int n = static_cast<int>(children_.size());
        for (int i = 0; i < n; ++i) {
            const float a = startAngle + 2.0f * PI * float(i) / float(n);
            Mat4 local = Mat4::translate2D(radius * cos(a), radius * sin(a));
            if (faceOutward) local = local * Mat4::rotateZ(a - 0.5f * PI);
            Child& ch = children_[static_cast<size_t>(i)];
            if (ch.shape) ch.shape->transform.matrix = local;
            else if (ch.group) ch.group->transform.matrix = local;
        }
    }

    void arrangeLinear(float spacing, float angleRad = 0.0f) {
        if (children_.empty()) return;
        const int n = static_cast<int>(children_.size());
        const float c = cos(angleRad);
        const float s = sin(angleRad);
        const float start = -0.5f * float(n - 1) * spacing;
        for (int i = 0; i < n; ++i) {
            const float t = start + float(i) * spacing;
            const Mat4 local = Mat4::translate2D(t * c, t * s);
            Child& ch = children_[static_cast<size_t>(i)];
            if (ch.shape) ch.shape->transform.matrix = local;
            else if (ch.group) ch.group->transform.matrix = local;
        }
    }

    void arrangeGrid(int cols, float dx, float dy) {
        if (children_.empty() || cols < 1) return;
        const int n    = static_cast<int>(children_.size());
        const int rows = (n + cols - 1) / cols;
        const float x0 = -0.5f * float(cols - 1) * dx;
        const float y0 =  0.5f * float(rows - 1) * dy;
        for (int i = 0; i < n; ++i) {
            const int r = i / cols;
            const int c = i % cols;
            const Mat4 local =
                Mat4::translate2D(x0 + float(c) * dx, y0 - float(r) * dy);
            Child& ch = children_[static_cast<size_t>(i)];
            if (ch.shape) ch.shape->transform.matrix = local;
            else if (ch.group) ch.group->transform.matrix = local;
        }
    }

private:
    class Child {
    public:
        Shape* shape = nullptr;
        ShapeGroup* group = nullptr;
    };
    vector<Child> children_;
    vector<unique_ptr<Shape>> owned_;
};


#endif // GL2D_H_INCLUDED
