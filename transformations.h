// =============================================================================
//  transform_math.h — 2D transform math for OpenGL-style column-major matrices
//
//  No OpenGL headers: safe to include anywhere you need Mat4 / TRS only.
//  Convention matches glUniformMatrix4fv(..., GL_FALSE, ptr).
// =============================================================================

#ifndef TRANSFORM_MATH_H_INCLUDED
#define TRANSFORM_MATH_H_INCLUDED

#include <cmath>

constexpr float PI = 3.14159265358979323846f;

// -----------------------------------------------------------------------------
//  Mat4 — column-major 4x4
//
//  Indexing: columns are contiguous — m[0..3] = column 0, m[4..7] = column 1, ...
//  For a column vector p, transformed position is: p' = M * p (standard GL).
//
//  2D helpers use the XY plane (Z rotation).  Compose as: T * R * S so that scale
//  applies in local space, then rotation, then translation in world space:
//      p_world = T * R * S * p_local
//
//  3D helpers follow the same convention (column vectors, right-hand rule):
//      p_world = T * Rz * Ry * Rx * S * p_local  (one common order)
class Mat4 {
public:
    float m[16]{};

    static Mat4 identity() {
        Mat4 r{};
        r.m[0]  = 1.0f;
        r.m[5]  = 1.0f;
        r.m[10] = 1.0f;
        r.m[15] = 1.0f;
        return r;
    }

    // Translation in the XY plane (Z unchanged).
    static Mat4 translate2D(float tx, float ty) {
        Mat4 r = identity();
        r.m[12] = tx;
        r.m[13] = ty;
        return r;
    }

    // Counter-clockwise rotation around +Z (right-hand rule), radians.
    static Mat4 rotateZ(float rad) {
        const float c = std::cos(rad);
        const float s = std::sin(rad);
        Mat4 r  = identity();
        r.m[0] = c;
        r.m[1] = s;
        r.m[4] = -s;
        r.m[5] = c;
        return r;
    }

    // Non-uniform scale in X and Y (Z = 1).
    static Mat4 scale2D(float sx, float sy) {
        Mat4 r = identity();
        r.m[0]  = sx;
        r.m[5]  = sy;
        return r;
    }

    static Mat4 scaleUniform(float s) { return scale2D(s, s); }

    // Translation in XYZ.
    static Mat4 translate3D(float tx, float ty, float tz) {
        Mat4 r = identity();
        r.m[12] = tx;
        r.m[13] = ty;
        r.m[14] = tz;
        return r;
    }

    // Rotation around +X, radians (right-hand rule).
    static Mat4 rotateX(float rad) {
        const float c = std::cos(rad);
        const float s = std::sin(rad);
        Mat4 r = identity();
        r.m[5]  = c;
        r.m[6]  = s;
        r.m[9]  = -s;
        r.m[10] = c;
        return r;
    }

    // Rotation around +Y, radians (right-hand rule).
    static Mat4 rotateY(float rad) {
        const float c = std::cos(rad);
        const float s = std::sin(rad);
        Mat4 r = identity();
        r.m[0]  = c;
        r.m[2]  = -s;
        r.m[8]  = s;
        r.m[10] = c;
        return r;
    }

    // Non-uniform scale in XYZ.
    static Mat4 scale3D(float sx, float sy, float sz) {
        Mat4 r = identity();
        r.m[0]  = sx;
        r.m[5]  = sy;
        r.m[10] = sz;
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 c{};
        for (int j = 0; j < 4; ++j) {
            for (int i = 0; i < 4; ++i) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                    sum += m[k * 4 + i] * b.m[j * 4 + k];
                c.m[j * 4 + i] = sum;
            }
        }
        return c;
    }

    const float* data() const { return m; }
};

// -----------------------------------------------------------------------------
//  Quaternion helpers (for safer orientation accumulation than Euler angles)
// -----------------------------------------------------------------------------
class Quat {
public:
    float w, x, y, z;

    static Quat identity() { return {1.0f, 0.0f, 0.0f, 0.0f}; }

    // Axis-angle quaternion: axis does not need to be normalized.
    static Quat fromAxisAngle(float ax, float ay, float az, float rad) {
        const float len = std::sqrt(ax * ax + ay * ay + az * az);
        if (len < 1e-8f) return identity();
        const float invLen = 1.0f / len;
        const float h = 0.5f * rad;
        const float s = std::sin(h);
        return {std::cos(h), ax * invLen * s, ay * invLen * s, az * invLen * s};
    }

    Quat normalized() const {
        const float n = std::sqrt(w * w + x * x + y * y + z * z);
        if (n < 1e-8f) return identity();
        const float invN = 1.0f / n;
        return {w * invN, x * invN, y * invN, z * invN};
    }
};

// Hamilton product (composition): qTotal = b * a applies a then b.
inline Quat operator*(const Quat& a, const Quat& b) {
    return {
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
    };
}

// Convert unit quaternion to Mat4 rotation (column-major, same convention as Mat4).
inline Mat4 mat4FromQuat(const Quat& qIn) {
    const Quat q = qIn.normalized();
    const float w = q.w, x = q.x, y = q.y, z = q.z;

    Mat4 r = Mat4::identity();
    r.m[0]  = 1.0f - 2.0f * (y * y + z * z);
    r.m[1]  = 2.0f * (x * y + w * z);
    r.m[2]  = 2.0f * (x * z - w * y);

    r.m[4]  = 2.0f * (x * y - w * z);
    r.m[5]  = 1.0f - 2.0f * (x * x + z * z);
    r.m[6]  = 2.0f * (y * z + w * x);

    r.m[8]  = 2.0f * (x * z + w * y);
    r.m[9]  = 2.0f * (y * z - w * x);
    r.m[10] = 1.0f - 2.0f * (x * x + y * y);
    return r;
}

inline Mat4 rotateAxisAngle(float ax, float ay, float az, float rad) {
    return mat4FromQuat(Quat::fromAxisAngle(ax, ay, az, rad));
}

//2D TRS order T * R * S
inline Mat4 trs2D(float tx, float ty, float rotationZRad, float uniformScale) {
    return Mat4::translate2D(tx, ty) * Mat4::rotateZ(rotationZRad) * Mat4::scaleUniform(uniformScale);
}

//3D TRS order T * Rz * Ry * Rx * S.
inline Mat4 trs3D(float tx, float ty, float tz,
                  float rotXRad, float rotYRad, float rotZRad,
                  float sx, float sy, float sz) {
    return Mat4::translate3D(tx, ty, tz) *
           Mat4::rotateZ(rotZRad) *
           Mat4::rotateY(rotYRad) *
           Mat4::rotateX(rotXRad) *
           Mat4::scale3D(sx, sy, sz);
}


class Transform {
public:
    Mat4 matrix = Mat4::identity();
};

#endif
