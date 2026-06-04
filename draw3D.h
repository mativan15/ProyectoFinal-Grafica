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

// -----------------------------------------------------------------------------
//  GenShape
//  Generic shape that uploads user-provided geometry and renders it.
//  - Supports both 2D and 3D positions:
//      dims = 2  -> input is [x,y, x,y, ...], z is set to 0
//      dims = 3  -> input is [x,y,z, x,y,z, ...]
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

    // Update geometry at runtime.
    bool setGeometry(const vector<float>& vertices,
                     int dims = 3,
                     const vector<unsigned int>& indices = {},
                     GLenum primitiveMode = GL_TRIANGLES) {
        if (dims != 2 && dims != 3) {
            fprintf(stderr, "draw3D: GenShape dims must be 2 or 3 (got %d)\n", dims);
            return false;
        }
        if (vertices.empty() || (vertices.size() % static_cast<size_t>(dims)) != 0) {
            fprintf(stderr,
                    "draw3D: GenShape vertices size (%zu) is not compatible with dims=%d\n",
                    vertices.size(), dims);
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
        return true;
    }
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

        if (primMode_ == DrawPrim::Filled && useFaceColors_) {
            const float P[8][3] = {
                {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {-hx, hy, -hz},
                {-hx, -hy,  hz}, {hx, -hy,  hz}, {hx, hy,  hz}, {-hx, hy,  hz}
            };
            static const int faceCorners[6][4] = {
                {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 4, 5, 1},
                {3, 2, 6, 7}, {0, 3, 7, 4}, {1, 5, 6, 2}
            };
            vector<float> interleaved;
            interleaved.reserve(24 * 7);
            for (int f = 0; f < 6; ++f) {
                const Color& c = faceColors[static_cast<size_t>(f)];
                for (int k = 0; k < 4; ++k) {
                    const int vi = faceCorners[f][k];
                    interleaved.push_back(P[vi][0]);
                    interleaved.push_back(P[vi][1]);
                    interleaved.push_back(P[vi][2]);
                    interleaved.push_back(c.r);
                    interleaved.push_back(c.g);
                    interleaved.push_back(c.b);
                    interleaved.push_back(c.a);
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
            uploadWithColor(interleaved, idx);
            return;
        }

        vector<unsigned int> idx;
        if (primMode_ == DrawPrim::Filled) {
            idx = {
                0, 1, 2,  0, 2, 3,  // back
                4, 6, 5,  4, 7, 6,  // front
                0, 4, 5,  0, 5, 1,  // bottom
                3, 2, 6,  3, 6, 7,  // top
                0, 3, 7,  0, 7, 4,  // left
                1, 5, 6,  1, 6, 2   // right
            };
            primitive = GL_TRIANGLES;
        } else {
            idx = {
                0, 1, 1, 2, 2, 3, 3, 0,  // back loop
                4, 5, 5, 6, 6, 7, 7, 4,  // front loop
                0, 4, 1, 5, 2, 6, 3, 7   // connectors
            };
            primitive = GL_LINES;
        }
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

        if (primMode_ == DrawPrim::Filled && useFaceColors_) {
            vector<float> interleaved;
            interleaved.reserve(16 * 7);

            const Color& cBase = faceColors[0];
            const float basePts[4][3] = {
                {-hx, yBase, -hz}, {hx, yBase, -hz}, {hx, yBase, hz}, {-hx, yBase, hz}
            };
            for (int i = 0; i < 4; ++i) {
                interleaved.push_back(basePts[i][0]);
                interleaved.push_back(basePts[i][1]);
                interleaved.push_back(basePts[i][2]);
                interleaved.push_back(cBase.r);
                interleaved.push_back(cBase.g);
                interleaved.push_back(cBase.b);
                interleaved.push_back(cBase.a);
            }

            const float sidePts[4][3][3] = {
                {{-hx, yBase, -hz}, { hx, yBase, -hz}, {0, yApex, 0}},
                {{ hx, yBase, -hz}, { hx, yBase,  hz}, {0, yApex, 0}},
                {{ hx, yBase,  hz}, {-hx, yBase,  hz}, {0, yApex, 0}},
                {{-hx, yBase,  hz}, {-hx, yBase, -hz}, {0, yApex, 0}}
            };
            for (int f = 0; f < 4; ++f) {
                const Color& c = faceColors[static_cast<size_t>(f + 1)];
                for (int k = 0; k < 3; ++k) {
                    interleaved.push_back(sidePts[f][k][0]);
                    interleaved.push_back(sidePts[f][k][1]);
                    interleaved.push_back(sidePts[f][k][2]);
                    interleaved.push_back(c.r);
                    interleaved.push_back(c.g);
                    interleaved.push_back(c.b);
                    interleaved.push_back(c.a);
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
            uploadWithColor(interleaved, idx);
            return;
        }

        vector<unsigned int> idx;
        if (primMode_ == DrawPrim::Filled) {
            idx = {
                0, 1, 2,  0, 2, 3,  // base
                0, 1, 4,            // sides
                1, 2, 4,
                2, 3, 4,
                3, 0, 4
            };
            primitive = GL_TRIANGLES;
        } else {
            idx = {
                0, 1, 1, 2, 2, 3, 3, 0,  // base edges
                0, 4, 1, 4, 2, 4, 3, 4   // side edges
            };
            primitive = GL_LINES;
        }
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

#endif // DRAW3D_H_INCLUDED
