// =============================================================================
//  draw3D.h - Simple 3D primitives built on top of draw2D.h
//
//  Reuses existing core infrastructure from draw2D.h:
//    - Shape (VAO/VBO/EBO upload and draw)
//    - Shader / defaultShader
//    - Color / Transform / Mat4
//
//  This header only adds geometry generators for 3D primitives:
//    - GenShape (generic geometry from raw vertices/indices, 2D or 3D)
//    - Cube (optional per-face colors via vector<Color> of size 6)
//    - Pyramid (optional per-face colors via vector<Color> of size 5)
//    - Sphere
//
//  Notes:
//  - The default shader from draw2D.h uses uModel only (no view/projection).
//    So 3D vertices still need to end up in clip-space [-1,1] unless you use a
//    custom shader with camera/projection matrices.
// =============================================================================

#ifndef DRAW3D_H_INCLUDED
#define DRAW3D_H_INCLUDED

#include "draw2D.h"

#include <array>
#include <fstream>
#include <limits>
#include <sstream>

class MeshVertex {
public:
    LightVec3 position {0.0f, 0.0f, 0.0f};
    Color color {1.0f, 1.0f, 1.0f, 1.0f};
    LightVec3 normal {0.0f, 0.0f, 1.0f};
    float u = 0.0f;
    float v = 0.0f;
};

inline LightVec3 makeVec3(float x, float y, float z) { return {x, y, z}; }

inline LightVec3 addVec3(const LightVec3& a, const LightVec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline LightVec3 subVec3(const LightVec3& a, const LightVec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline LightVec3 scaleVec3(const LightVec3& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}

inline float dotVec3(const LightVec3& a, const LightVec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline LightVec3 crossVec3(const LightVec3& a, const LightVec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline LightVec3 normalizeVec3(const LightVec3& v) {
    const float len = sqrt(dotVec3(v, v));
    if (len < 1e-8f) return {0.0f, 0.0f, 1.0f};
    return {v.x / len, v.y / len, v.z / len};
}

inline LightVec3 faceNormal(const LightVec3& a, const LightVec3& b, const LightVec3& c) {
    return normalizeVec3(crossVec3(subVec3(b, a), subVec3(c, a)));
}

inline void appendLitVertex(vector<float>& out, const MeshVertex& v) {
    out.push_back(v.position.x);
    out.push_back(v.position.y);
    out.push_back(v.position.z);
    out.push_back(v.color.r);
    out.push_back(v.color.g);
    out.push_back(v.color.b);
    out.push_back(v.color.a);
    out.push_back(v.normal.x);
    out.push_back(v.normal.y);
    out.push_back(v.normal.z);
    out.push_back(v.u);
    out.push_back(v.v);
}

inline vector<float> interleaveLitVertices(const vector<MeshVertex>& vertices) {
    vector<float> out;
    out.reserve(vertices.size() * 12);
    for (const MeshVertex& v : vertices) appendLitVertex(out, v);
    return out;
}

// -----------------------------------------------------------------------------
//  GenShape
//  Generic shape that uploads user-provided geometry or an OBJ file and renders it.
//  - Supports both 2D and 3D positions:
//      dims = 2  -> input is [x,y, x,y, ...], z is set to 0
//      dims = 3  -> input is [x,y,z, x,y,z, ...]
//  - Supports simple OBJ files with v/f records. Faces are triangulated.
//  - Optional index buffer for indexed drawing.
//  - primitive can be GL_TRIANGLES, GL_LINES, GL_POINTS, etc.
// -----------------------------------------------------------------------------
class GenShape : public Shape {
public:
    GenShape(const vector<float>& vertices,
             int dims = 3,
             const vector<unsigned int>& indices = {},
             GLenum primitiveMode = GL_TRIANGLES) {
        setGeometry(vertices, dims, indices, primitiveMode);
    }

    explicit GenShape(const string& objPath,
                      GLenum primitiveMode = GL_TRIANGLES) {
        setGeometryFromObj(objPath, primitiveMode);
    }

    // Convenience overload for 2D points.
    GenShape(const vector<pair<float, float>>& points2D,
             const vector<unsigned int>& indices = {},
             GLenum primitiveMode = GL_TRIANGLES) {
        vector<float> verts;
        verts.reserve(points2D.size() * 2);
        for (const auto& p : points2D) {
            verts.push_back(p.first);
            verts.push_back(p.second);
        }
        setGeometry(verts, 2, indices, primitiveMode);
    }

    GenShape(const vector<MeshVertex>& vertices,
             const vector<unsigned int>& indices = {},
             GLenum primitiveMode = GL_TRIANGLES) {
        setGeometry(vertices, indices, primitiveMode);
    }

    bool loaded() const { return loaded_; }
    const string& error() const { return error_; }

    // Update geometry at runtime.
    bool setGeometry(const vector<float>& vertices,
                     int dims = 3,
                     const vector<unsigned int>& indices = {},
                     GLenum primitiveMode = GL_TRIANGLES) {
        if (dims != 2 && dims != 3) {
            error_ = "GenShape dims must be 2 or 3";
            fprintf(stderr, "draw3D: %s (got %d)\n", error_.c_str(), dims);
            loaded_ = false;
            return false;
        }
        if (vertices.empty() || (vertices.size() % static_cast<size_t>(dims)) != 0) {
            error_ = "GenShape vertices size is not compatible with dims";
            fprintf(stderr,
                    "draw3D: GenShape vertices size (%zu) is not compatible with dims=%d\n",
                    vertices.size(), dims);
            loaded_ = false;
            return false;
        }

        vector<float> xyz;
        if (dims == 3) {
            xyz = vertices;
        } else {
            xyz.reserve((vertices.size() / 2) * 3);
            for (size_t i = 0; i < vertices.size(); i += 2) {
                xyz.push_back(vertices[i + 0]);
                xyz.push_back(vertices[i + 1]);
                xyz.push_back(0.0f);
            }
        }

        primitive = primitiveMode;
        upload(xyz, indices);
        loaded_ = true;
        error_.clear();
        return true;
    }

    bool setGeometry(const vector<MeshVertex>& vertices,
                     const vector<unsigned int>& indices = {},
                     GLenum primitiveMode = GL_TRIANGLES) {
        if (vertices.empty()) {
            error_ = "GenShape MeshVertex input is empty";
            fprintf(stderr, "draw3D: %s\n", error_.c_str());
            loaded_ = false;
            return false;
        }

        primitive = primitiveMode;
        uploadLit(interleaveLitVertices(vertices), indices, true);
        loaded_ = true;
        error_.clear();
        return true;
    }

    bool setGeometryFromObj(const string& objPath,
                            GLenum primitiveMode = GL_TRIANGLES) {
        ObjMeshData mesh;
        if (!loadObj(objPath, mesh)) {
            loaded_ = false;
            fprintf(stderr, "draw3D: GenShape OBJ load failed: %s\n", error_.c_str());
            return false;
        }

        primitive = primitiveMode;
        uploadLit(interleaveLitVertices(mesh.vertices), mesh.indices, mesh.hasTexCoords);
        loaded_ = true;
        error_.clear();
        return true;
    }

private:
    class ObjPosition {
    public:
        LightVec3 position {0.0f, 0.0f, 0.0f};
        Color color {1.0f, 1.0f, 1.0f, 1.0f};
    };

    class ObjTexCoord {
    public:
        float u = 0.0f;
        float v = 0.0f;
    };

    class ObjRef {
    public:
        int position = -1;
        int texCoord = -1;
        int normal = -1;
    };

    class ObjMeshData {
    public:
        vector<MeshVertex> vertices;
        vector<unsigned int> indices;
        bool hasTexCoords = false;
    };

    bool loadObj(const string& objPath, ObjMeshData& mesh) {
        string loadedPath;
        ifstream file = openObjFile(objPath, loadedPath);
        if (!file) {
            error_ = "could not open OBJ file: " + objPath;
            return false;
        }

        vector<ObjPosition> positions;
        vector<ObjTexCoord> texCoords;
        vector<LightVec3> normals;
        vector<array<ObjRef, 3>> triangles;

        LightVec3 minPos{
            numeric_limits<float>::max(),
            numeric_limits<float>::max(),
            numeric_limits<float>::max()
        };
        LightVec3 maxPos{
            numeric_limits<float>::lowest(),
            numeric_limits<float>::lowest(),
            numeric_limits<float>::lowest()
        };

        string line;
        int lineNumber = 0;
        while (getline(file, line)) {
            ++lineNumber;

            const size_t commentPos = line.find('#');
            if (commentPos != string::npos) {
                line = line.substr(0, commentPos);
            }

            string tag;
            stringstream ss(line);
            ss >> tag;

            if (tag == "v") {
                ObjPosition p{};
                if (!(ss >> p.position.x >> p.position.y >> p.position.z)) {
                    error_ = "invalid OBJ vertex at line " + to_string(lineNumber);
                    return false;
                }

                float r = 1.0f;
                float g = 1.0f;
                float b = 1.0f;
                float a = 1.0f;
                if (ss >> r >> g >> b) {
                    if (!(ss >> a)) a = 1.0f;
                    p.color = normalizeObjColor(r, g, b, a);
                }

                positions.push_back(p);
                minPos.x = min(minPos.x, p.position.x);
                minPos.y = min(minPos.y, p.position.y);
                minPos.z = min(minPos.z, p.position.z);
                maxPos.x = max(maxPos.x, p.position.x);
                maxPos.y = max(maxPos.y, p.position.y);
                maxPos.z = max(maxPos.z, p.position.z);
            } else if (tag == "vt") {
                ObjTexCoord uv{};
                if (!(ss >> uv.u)) {
                    error_ = "invalid OBJ texture coordinate at line " + to_string(lineNumber);
                    return false;
                }
                if (!(ss >> uv.v)) uv.v = 0.0f;
                texCoords.push_back(uv);
            } else if (tag == "vn") {
                LightVec3 n{};
                if (!(ss >> n.x >> n.y >> n.z)) {
                    error_ = "invalid OBJ normal at line " + to_string(lineNumber);
                    return false;
                }
                normals.push_back(normalizeVec3(n));
            } else if (tag == "f") {
                vector<ObjRef> face;
                string token;
                while (ss >> token) {
                    ObjRef ref{};
                    if (!parseObjFaceToken(token, positions.size(),
                                           texCoords.size(), normals.size(), ref)) {
                        error_ = "invalid OBJ face token at line " + to_string(lineNumber);
                        return false;
                    }
                    face.push_back(ref);
                }

                if (face.size() < 3) {
                    error_ = "OBJ face has fewer than 3 vertices at line " + to_string(lineNumber);
                    return false;
                }

                for (size_t i = 1; i + 1 < face.size(); ++i) {
                    triangles.push_back({face[0], face[i], face[i + 1]});
                }
            }
        }

        if (positions.empty()) {
            error_ = "OBJ has no vertices: " + loadedPath;
            return false;
        }
        if (triangles.empty()) {
            error_ = "OBJ has no faces: " + loadedPath;
            return false;
        }

        buildObjMesh(positions, texCoords, normals, triangles, minPos, maxPos, mesh);
        return true;
    }

    static void buildObjMesh(const vector<ObjPosition>& positions,
                             const vector<ObjTexCoord>& texCoords,
                             const vector<LightVec3>& normals,
                             const vector<array<ObjRef, 3>>& triangles,
                             const LightVec3& minPos,
                             const LightVec3& maxPos,
                             ObjMeshData& mesh) {
        const LightVec3 center{
            (minPos.x + maxPos.x) * 0.5f,
            (minPos.y + maxPos.y) * 0.5f,
            (minPos.z + maxPos.z) * 0.5f
        };
        const LightVec3 size{
            maxPos.x - minPos.x,
            maxPos.y - minPos.y,
            maxPos.z - minPos.z
        };
        const float maxExtent = max(size.x, max(size.y, size.z));
        const float scale = maxExtent > 0.0f ? 2.0f / maxExtent : 1.0f;

        mesh.vertices.clear();
        mesh.indices.clear();
        mesh.vertices.reserve(triangles.size() * 3);
        mesh.hasTexCoords = false;

        for (const array<ObjRef, 3>& triRef : triangles) {
            MeshVertex tri[3];
            for (int i = 0; i < 3; ++i) {
                const ObjRef& ref = triRef[static_cast<size_t>(i)];
                const ObjPosition& src = positions[static_cast<size_t>(ref.position)];
                tri[i].position = {
                    (src.position.x - center.x) * scale,
                    (src.position.y - center.y) * scale,
                    (src.position.z - center.z) * scale
                };
                tri[i].color = src.color;
                if (ref.texCoord >= 0 && ref.texCoord < static_cast<int>(texCoords.size())) {
                    const ObjTexCoord& uv = texCoords[static_cast<size_t>(ref.texCoord)];
                    tri[i].u = uv.u;
                    tri[i].v = uv.v;
                    mesh.hasTexCoords = true;
                }
                if (ref.normal >= 0 && ref.normal < static_cast<int>(normals.size())) {
                    tri[i].normal = normals[static_cast<size_t>(ref.normal)];
                }
            }

            const LightVec3 generatedNormal =
                faceNormal(tri[0].position, tri[1].position, tri[2].position);
            for (int i = 0; i < 3; ++i) {
                if (triRef[static_cast<size_t>(i)].normal < 0) {
                    tri[i].normal = generatedNormal;
                } else {
                    tri[i].normal = normalizeVec3(tri[i].normal);
                }
            }

            for (int i = 0; i < 3; ++i) {
                mesh.vertices.push_back(tri[i]);
            }
        }
    }

    static Color normalizeObjColor(float r, float g, float b, float a) {
        const float maxRgb = max(r, max(g, b));
        if (maxRgb > 1.0f) {
            return {r / 255.0f, g / 255.0f, b / 255.0f, a > 1.0f ? a / 255.0f : a};
        }
        return {r, g, b, a > 1.0f ? a / 255.0f : a};
    }

    static ifstream openObjFile(const string& objPath, string& loadedPath) {
        ifstream direct(objPath);
        if (direct) {
            loadedPath = objPath;
            return direct;
        }

        const string sourceDir = headerDirectory();
        if (sourceDir != ".") {
            const string sourceRelative = sourceDir + "/" + objPath;
            ifstream fromHeader(sourceRelative);
            if (fromHeader) {
                loadedPath = sourceRelative;
                return fromHeader;
            }
        }

        return ifstream();
    }

    static string headerDirectory() {
        string path = __FILE__;
        const size_t slashPos = path.find_last_of("/\\");
        if (slashPos == string::npos) return ".";
        return path.substr(0, slashPos);
    }

    static bool parseObjFaceToken(const string& token,
                                  size_t positionCount,
                                  size_t texCoordCount,
                                  size_t normalCount,
                                  ObjRef& out) {
        const size_t firstSlash = token.find('/');
        const size_t secondSlash = firstSlash == string::npos
            ? string::npos
            : token.find('/', firstSlash + 1);

        const string positionText = firstSlash == string::npos
            ? token
            : token.substr(0, firstSlash);
        const string texCoordText = firstSlash == string::npos
            ? string()
            : (secondSlash == string::npos
                ? token.substr(firstSlash + 1)
                : token.substr(firstSlash + 1, secondSlash - firstSlash - 1));
        const string normalText = secondSlash == string::npos
            ? string()
            : token.substr(secondSlash + 1);

        out.position = parseObjIndex(positionText, positionCount);
        if (out.position < 0) return false;
        if (!texCoordText.empty()) {
            out.texCoord = parseObjIndex(texCoordText, texCoordCount);
            if (out.texCoord < 0) return false;
        }
        if (!normalText.empty()) {
            out.normal = parseObjIndex(normalText, normalCount);
            if (out.normal < 0) return false;
        }
        return true;
    }

    static int parseObjIndex(const string& text, size_t count) {
        if (text.empty()) return -1;

        int rawIndex = 0;
        try {
            size_t used = 0;
            rawIndex = stoi(text, &used);
            if (used != text.size()) return -1;
        } catch (...) {
            return -1;
        }

        if (rawIndex > 0) {
            const int idx = rawIndex - 1;
            return idx < static_cast<int>(count) ? idx : -1;
        }
        if (rawIndex < 0) {
            const int idx = static_cast<int>(count) + rawIndex;
            return idx >= 0 && idx < static_cast<int>(count) ? idx : -1;
        }
        return -1;
    }

    bool loaded_ = false;
    string error_;
};

// -----------------------------------------------------------------------------
//  Cube centered at origin.
//  size = full edge length.
// -----------------------------------------------------------------------------
class Cube : public Shape {
public:
    // Runtime-editable per-face colors for filled cubes.
    // Face order: back, front, bottom, top, left, right.
    vector<Color> faceColors;

    Cube(float width, float height, float depth, const vector<Color>& faceColors,
         DrawPrim prim = DrawPrim::Filled)
        : width_(width), height_(height), depth_(depth), primMode_(prim),
          faceColors(faceColors), useFaceColors_(true) {
        if (!validateFaceColors(faceColors)) {
            useFaceColors_ = false;
        }
        rebuildGeometry();
    }

    Cube(float size, const vector<Color>& faceColors, DrawPrim prim = DrawPrim::Filled)
        : Cube(size, size, size, faceColors, prim) {}

    Cube(float width, float height, float depth, DrawPrim prim = DrawPrim::Filled)
        : width_(width), height_(height), depth_(depth), primMode_(prim) {
        rebuildGeometry();
    }

    Cube(float size = 1.0f, DrawPrim prim = DrawPrim::Filled)
        : Cube(size, size, size, prim) {}

    bool setFaceColors(const vector<Color>& newFaceColors) {
        if (!validateFaceColors(newFaceColors)) return false;
        faceColors = newFaceColors;
        useFaceColors_ = true;
        rebuildGeometry();
        return true;
    }

    // Call this after directly editing `faceColors`.
    bool applyFaceColors() {
        if (!validateFaceColors(faceColors)) return false;
        useFaceColors_ = true;
        rebuildGeometry();
        return true;
    }

    void clearFaceColors() {
        useFaceColors_ = false;
        faceColors.clear();
        rebuildGeometry();
    }

private:
    float width_ = 1.0f;
    float height_ = 1.0f;
    float depth_ = 1.0f;
    DrawPrim primMode_ = DrawPrim::Filled;
    bool useFaceColors_ = false;

    static bool validateFaceColors(const vector<Color>& colorsIn) {
        if (colorsIn.size() != 6) {
            fprintf(stderr, "draw3D: Cube faceColors must have size 6 (got %zu)\n",
                    colorsIn.size());
            return false;
        }
        return true;
    }

    void rebuildGeometry() {
        const float hx = width_ * 0.5f;
        const float hy = height_ * 0.5f;
        const float hz = depth_ * 0.5f;
        vector<float> verts = {
            -hx, -hy, -hz,  // 0
             hx, -hy, -hz,  // 1
             hx,  hy, -hz,  // 2
            -hx,  hy, -hz,  // 3
            -hx, -hy,  hz,  // 4
             hx, -hy,  hz,  // 5
             hx,  hy,  hz,  // 6
            -hx,  hy,  hz   // 7
        };

        if (primMode_ == DrawPrim::Vertices) {
            primitive = GL_POINTS;
            pointSize = 4.0f;
            upload(verts);
            return;
        }

        if (primMode_ == DrawPrim::Filled) {
            const float P[8][3] = {
                {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {-hx, hy, -hz},
                {-hx, -hy,  hz}, {hx, -hy,  hz}, {hx, hy,  hz}, {-hx, hy,  hz}
            };
            static const int faceCorners[6][4] = {
                {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 4, 5, 1},
                {3, 2, 6, 7}, {0, 3, 7, 4}, {1, 5, 6, 2}
            };
            static const LightVec3 normals[6] = {
                { 0.0f,  0.0f, -1.0f}, { 0.0f,  0.0f,  1.0f},
                { 0.0f, -1.0f,  0.0f}, { 0.0f,  1.0f,  0.0f},
                {-1.0f,  0.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}
            };
            static const float uv[4][2] = {
                {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
            };
            vector<float> interleaved;
            interleaved.reserve(24 * 12);
            for (int f = 0; f < 6; ++f) {
                const Color c = useFaceColors_ ? faceColors[static_cast<size_t>(f)] : colors::White;
                for (int k = 0; k < 4; ++k) {
                    const int vi = faceCorners[f][k];
                    appendLitVertex(interleaved, {
                        {P[vi][0], P[vi][1], P[vi][2]},
                        c,
                        normals[f],
                        uv[k][0], uv[k][1]
                    });
                }
            }
            vector<unsigned int> idx;
            idx.reserve(36);
            for (int f = 0; f < 6; ++f) {
                const unsigned int b = static_cast<unsigned int>(f * 4);
                idx.push_back(b + 0); idx.push_back(b + 1); idx.push_back(b + 2);
                idx.push_back(b + 0); idx.push_back(b + 2); idx.push_back(b + 3);
            }
            primitive = GL_TRIANGLES;
            uploadLit(interleaved, idx);
            return;
        }

        vector<unsigned int> idx;
        idx = {
            0, 1, 1, 2, 2, 3, 3, 0,  // back loop
            4, 5, 5, 6, 6, 7, 7, 4,  // front loop
            0, 4, 1, 5, 2, 6, 3, 7   // connectors
        };
        primitive = GL_LINES;
        upload(verts, idx);
    }
};

// -----------------------------------------------------------------------------
//  Square-base pyramid centered around origin in XZ, with apex at +Y.
// -----------------------------------------------------------------------------
class Pyramid : public Shape {
public:
    // Runtime-editable per-face colors for filled pyramids.
    // Face order: base, side(0-1-apex), side(1-2-apex), side(2-3-apex), side(3-0-apex).
    vector<Color> faceColors;

    Pyramid(float baseWidth, float baseDepth, float height, const vector<Color>& faceColors,
            DrawPrim prim = DrawPrim::Filled)
        : baseWidth_(baseWidth), baseDepth_(baseDepth), height_(height), primMode_(prim),
          faceColors(faceColors), useFaceColors_(true) {
        if (!validateFaceColors(faceColors)) {
            useFaceColors_ = false;
        }
        rebuildGeometry();
    }

    Pyramid(float baseSize, float height, const vector<Color>& faceColors,
            DrawPrim prim = DrawPrim::Filled)
        : Pyramid(baseSize, baseSize, height, faceColors, prim) {}

    Pyramid(float baseWidth, float baseDepth, float height, DrawPrim prim = DrawPrim::Filled)
        : baseWidth_(baseWidth), baseDepth_(baseDepth), height_(height), primMode_(prim) {
        rebuildGeometry();
    }

    Pyramid(float baseSize = 1.0f, float height = 1.0f,
            DrawPrim prim = DrawPrim::Filled)
        : Pyramid(baseSize, baseSize, height, prim) {}

    bool setFaceColors(const vector<Color>& newFaceColors) {
        if (!validateFaceColors(newFaceColors)) return false;
        faceColors = newFaceColors;
        useFaceColors_ = true;
        rebuildGeometry();
        return true;
    }

    // Call this after directly editing `faceColors`.
    bool applyFaceColors() {
        if (!validateFaceColors(faceColors)) return false;
        useFaceColors_ = true;
        rebuildGeometry();
        return true;
    }

    void clearFaceColors() {
        useFaceColors_ = false;
        faceColors.clear();
        rebuildGeometry();
    }

private:
    float baseWidth_ = 1.0f;
    float baseDepth_ = 1.0f;
    float height_ = 1.0f;
    DrawPrim primMode_ = DrawPrim::Filled;
    bool useFaceColors_ = false;

    static bool validateFaceColors(const vector<Color>& colorsIn) {
        if (colorsIn.size() != 5) {
            fprintf(stderr, "draw3D: Pyramid faceColors must have size 5 (got %zu)\n",
                    colorsIn.size());
            return false;
        }
        return true;
    }

    void rebuildGeometry() {
        const float hx = baseWidth_ * 0.5f;
        const float hz = baseDepth_ * 0.5f;
        const float yBase = -height_ * 0.5f;
        const float yApex =  height_ * 0.5f;

        vector<float> verts = {
            -hx, yBase, -hz,  // 0
             hx, yBase, -hz,  // 1
             hx, yBase,  hz,  // 2
            -hx, yBase,  hz,  // 3
             0, yApex,  0   // 4 apex
        };

        if (primMode_ == DrawPrim::Vertices) {
            primitive = GL_POINTS;
            pointSize = 4.0f;
            upload(verts);
            return;
        }

        if (primMode_ == DrawPrim::Filled) {
            vector<float> interleaved;
            interleaved.reserve(16 * 12);

            const Color cBase = useFaceColors_ ? faceColors[0] : colors::White;
            const float basePts[4][3] = {
                {-hx, yBase, -hz}, {hx, yBase, -hz}, {hx, yBase, hz}, {-hx, yBase, hz}
            };
            const float baseUv[4][2] = {
                {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
            };
            for (int i = 0; i < 4; ++i) {
                appendLitVertex(interleaved, {
                    {basePts[i][0], basePts[i][1], basePts[i][2]},
                    cBase,
                    {0.0f, -1.0f, 0.0f},
                    baseUv[i][0], baseUv[i][1]
                });
            }

            const float sidePts[4][3][3] = {
                {{-hx, yBase, -hz}, { hx, yBase, -hz}, {0, yApex, 0}},
                {{ hx, yBase, -hz}, { hx, yBase,  hz}, {0, yApex, 0}},
                {{ hx, yBase,  hz}, {-hx, yBase,  hz}, {0, yApex, 0}},
                {{-hx, yBase,  hz}, {-hx, yBase, -hz}, {0, yApex, 0}}
            };
            for (int f = 0; f < 4; ++f) {
                const Color c = useFaceColors_ ? faceColors[static_cast<size_t>(f + 1)] : colors::White;
                const LightVec3 p0 = {sidePts[f][0][0], sidePts[f][0][1], sidePts[f][0][2]};
                const LightVec3 p1 = {sidePts[f][1][0], sidePts[f][1][1], sidePts[f][1][2]};
                const LightVec3 p2 = {sidePts[f][2][0], sidePts[f][2][1], sidePts[f][2][2]};
                const LightVec3 n = faceNormal(p0, p1, p2);
                const float sideUv[3][2] = {
                    {0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}
                };
                for (int k = 0; k < 3; ++k) {
                    appendLitVertex(interleaved, {
                        {sidePts[f][k][0], sidePts[f][k][1], sidePts[f][k][2]},
                        c,
                        n,
                        sideUv[k][0], sideUv[k][1]
                    });
                }
            }

            vector<unsigned int> idx = {
                0, 1, 2,  0, 2, 3,   // base
                4, 5, 6,             // side 1
                7, 8, 9,             // side 2
                10, 11, 12,          // side 3
                13, 14, 15           // side 4
            };

            primitive = GL_TRIANGLES;
            uploadLit(interleaved, idx);
            return;
        }

        vector<unsigned int> idx;
        idx = {
            0, 1, 1, 2, 2, 3, 3, 0,  // base edges
            0, 4, 1, 4, 2, 4, 3, 4   // side edges
        };
        primitive = GL_LINES;
        upload(verts, idx);
    }
};

// -----------------------------------------------------------------------------
//  UV sphere centered at origin.
//  stacks: latitude bands, slices: longitude bands.
// -----------------------------------------------------------------------------
class Sphere : public Shape {
public:
    Sphere(float radiusX, float radiusY, float radiusZ, int stacks = 16, int slices = 24,
           DrawPrim prim = DrawPrim::Filled) {
        if (stacks < 2) stacks = 2;
        if (slices < 3) slices = 3;

        vector<float> verts;
        verts.reserve(static_cast<size_t>((stacks + 1) * (slices + 1) * 3));
        vector<float> litVerts;
        litVerts.reserve(static_cast<size_t>((stacks + 1) * (slices + 1) * 12));

        for (int i = 0; i <= stacks; ++i) {
            const float v = float(i) / float(stacks);
            const float phi = PI * v;  // 0..PI
            const float y = radiusY * cos(phi);
            const float r = sin(phi);

            for (int j = 0; j <= slices; ++j) {
                const float u = float(j) / float(slices);
                const float theta = 2.0f * PI * u;  // 0..2PI
                const float x = radiusX * r * cos(theta);
                const float z = radiusZ * r * sin(theta);
                verts.push_back(x);
                verts.push_back(y);
                verts.push_back(z);

                const LightVec3 normal = normalizeVec3({
                    radiusX != 0.0f ? x / (radiusX * radiusX) : 0.0f,
                    radiusY != 0.0f ? y / (radiusY * radiusY) : 0.0f,
                    radiusZ != 0.0f ? z / (radiusZ * radiusZ) : 0.0f
                });
                appendLitVertex(litVerts, {
                    {x, y, z},
                    colors::White,
                    normal,
                    u, 1.0f - v
                });
            }
        }

        const int row = slices + 1;

        if (prim == DrawPrim::Vertices) {
            primitive = GL_POINTS;
            pointSize = 4.0f;
            upload(verts);
        } else {
            vector<unsigned int> idx;
            if (prim == DrawPrim::Filled) {
                idx.reserve(static_cast<size_t>(stacks * slices * 6));
                for (int i = 0; i < stacks; ++i) {
                    for (int j = 0; j < slices; ++j) {
                        const unsigned int a = static_cast<unsigned int>(i * row + j);
                        const unsigned int b = static_cast<unsigned int>((i + 1) * row + j);
                        const unsigned int c = static_cast<unsigned int>((i + 1) * row + (j + 1));
                        const unsigned int d = static_cast<unsigned int>(i * row + (j + 1));
                        idx.push_back(a); idx.push_back(b); idx.push_back(c);
                        idx.push_back(a); idx.push_back(c); idx.push_back(d);
                    }
                }
                primitive = GL_TRIANGLES;
                uploadLit(litVerts, idx);
                return;
            } else {
                // Latitude lines
                idx.reserve(static_cast<size_t>(stacks * slices * 4));
                for (int i = 0; i <= stacks; ++i) {
                    for (int j = 0; j < slices; ++j) {
                        const unsigned int a = static_cast<unsigned int>(i * row + j);
                        const unsigned int b = static_cast<unsigned int>(i * row + (j + 1));
                        idx.push_back(a); idx.push_back(b);
                    }
                }
                // Longitude lines
                for (int j = 0; j < slices; ++j) {
                    for (int i = 0; i < stacks; ++i) {
                        const unsigned int a = static_cast<unsigned int>(i * row + j);
                        const unsigned int b = static_cast<unsigned int>((i + 1) * row + j);
                        idx.push_back(a); idx.push_back(b);
                    }
                }
                primitive = GL_LINES;
            }
            upload(verts, idx);
        }
    }

    Sphere(float radius = 0.5f, int stacks = 16, int slices = 24,
           DrawPrim prim = DrawPrim::Filled)
        : Sphere(radius, radius, radius, stacks, slices, prim) {}
};

// -----------------------------------------------------------------------------
//  GlowHalo
//  Flat transparent disc for additive glow around lamps, neon, or windows.
//  Place/rotate it with transform.matrix and draw it with enableAdditiveBlending().
// -----------------------------------------------------------------------------
class GlowHalo : public Shape {
public:
    GlowHalo(float radius = 1.0f,
             int segments = 48,
             Color centerColor = {1.0f, 0.1f, 0.8f, 0.55f},
             Color edgeColor = {1.0f, 0.1f, 0.8f, 0.0f}) {
        if (segments < 3) segments = 3;

        vector<float> interleaved;
        interleaved.reserve(static_cast<size_t>(segments + 2) * 7);
        appendVertex(interleaved, 0.0f, 0.0f, 0.0f, centerColor);
        for (int i = 0; i <= segments; ++i) {
            const float a = 2.0f * PI * float(i) / float(segments);
            appendVertex(interleaved, radius * cos(a), radius * sin(a), 0.0f, edgeColor);
        }

        primitive = GL_TRIANGLE_FAN;
        lightingEnabled = false;
        uploadWithColor(interleaved);
    }

private:
    static void appendVertex(vector<float>& out, float x, float y, float z, Color c) {
        out.push_back(x);
        out.push_back(y);
        out.push_back(z);
        out.push_back(c.r);
        out.push_back(c.g);
        out.push_back(c.b);
        out.push_back(c.a);
    }
};

#endif // DRAW3D_H_INCLUDED
