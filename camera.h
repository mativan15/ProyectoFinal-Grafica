#ifndef PROJECT11_CAMERA_H_INCLUDED
#define PROJECT11_CAMERA_H_INCLUDED

#include "transformations.h"

#include <cmath>

// -----------------------------------------------------------------------------
// Minimal vector math for camera/view calculations.
// -----------------------------------------------------------------------------
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(const Vec3& v, float s) { return {v.x * s, v.y * s, v.z * s}; }
inline Vec3 operator/(const Vec3& v, float s) { return {v.x / s, v.y / s, v.z / s}; }

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }

inline Vec3 normalize(const Vec3& v) {
    const float len = length(v);
    if (len < 1e-8f) return {0.0f, 0.0f, 0.0f};
    return v / len;
}

inline float radians(float deg) {
    return deg * (PI / 180.0f);
}

// -----------------------------------------------------------------------------
// gluLookAt equivalent (column-major, OpenGL style).
// Returns view matrix that transforms world -> camera space.
// -----------------------------------------------------------------------------
inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 f = normalize(center - eye);     // forward
    const Vec3 s = normalize(cross(f, up));     // right
    const Vec3 u = cross(s, f);                 // corrected up

    Mat4 view = Mat4::identity();
    view.m[0] = s.x;   view.m[1] = s.y;   view.m[2] = s.z;
    view.m[4] = u.x;   view.m[5] = u.y;   view.m[6] = u.z;
    view.m[8] = -f.x;  view.m[9] = -f.y;  view.m[10] = -f.z;

    view.m[12] = -dot(s, eye);
    view.m[13] = -dot(u, eye);
    view.m[14] =  dot(f, eye);
    return view;
}

// Alias with familiar OpenGL naming.
inline Mat4 gluLookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    return lookAt(eye, center, up);
}

// -----------------------------------------------------------------------------
// gluPerspective equivalent.
// fovyDeg: vertical FOV in degrees, aspect: width/height.
// -----------------------------------------------------------------------------
inline Mat4 perspective(float fovyDeg, float aspect, float zNear, float zFar) {
    const float f = 1.0f / std::tan(radians(fovyDeg) * 0.5f);

    Mat4 p{};
    p.m[0] = f / aspect;
    p.m[5] = f;
    p.m[10] = (zFar + zNear) / (zNear - zFar);
    p.m[11] = -1.0f;
    p.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    return p;
}

inline Mat4 gluPerspective(float fovyDeg, float aspect, float zNear, float zFar) {
    return perspective(fovyDeg, aspect, zNear, zFar);
}

// -----------------------------------------------------------------------------
// Orthographic projection equivalent.
// left/right/bottom/top define the visible box in world units.
// -----------------------------------------------------------------------------
inline Mat4 orthographic(float left, float right,
                        float bottom, float top,
                        float zNear, float zFar) {
    Mat4 p = Mat4::identity();

    p.m[0] = 2.0f / (right - left);
    p.m[5] = 2.0f / (top - bottom);
    p.m[10] = -2.0f / (zFar - zNear);

    p.m[12] = -(right + left) / (right - left);
    p.m[13] = -(top + bottom) / (top - bottom);
    p.m[14] = -(zFar + zNear) / (zFar - zNear);
    return p;
}

inline Mat4 ortho2D(float left, float right, float bottom, float top) {
    return orthographic(left, right, bottom, top, -1.0f, 1.0f);
}

// -----------------------------------------------------------------------------
// Simple FPS-like camera helper.
// -----------------------------------------------------------------------------
enum class CameraMove { Forward, Backward, Left, Right, Up, Down };

struct Camera {
    Vec3 position{0.0f, 0.0f, 2.0f};
    Vec3 front{0.0f, 0.0f, -1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    Vec3 right{1.0f, 0.0f, 0.0f};
    Vec3 worldUp{0.0f, 1.0f, 0.0f};

    float yawDeg = -90.0f;
    float pitchDeg = 0.0f;
    float moveSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
    float zoomFovDeg = 45.0f;

    Camera() { updateVectors(); }
    explicit Camera(const Vec3& startPos) : position(startPos) { updateVectors(); }

    Mat4 viewMatrix() const {
        return lookAt(position, position + front, up);
    }

    void move(CameraMove dir, float deltaTime) {
        const float v = moveSpeed * deltaTime;
        if (dir == CameraMove::Forward)  position = position + front * v;
        if (dir == CameraMove::Backward) position = position - front * v;
        if (dir == CameraMove::Left)     position = position - right * v;
        if (dir == CameraMove::Right)    position = position + right * v;
        if (dir == CameraMove::Up)       position = position + worldUp * v;
        if (dir == CameraMove::Down)     position = position - worldUp * v;
    }

    void rotateFromMouse(float xOffset, float yOffset, bool constrainPitch = true) {
        xOffset *= mouseSensitivity;
        yOffset *= mouseSensitivity;

        yawDeg += xOffset;
        pitchDeg += yOffset;

        if (constrainPitch) {
            if (pitchDeg > 89.0f) pitchDeg = 89.0f;
            if (pitchDeg < -89.0f) pitchDeg = -89.0f;
        }
        updateVectors();
    }

    void zoomFromScroll(float yOffset) {
        zoomFovDeg -= yOffset;
        if (zoomFovDeg < 1.0f) zoomFovDeg = 1.0f;
        if (zoomFovDeg > 90.0f) zoomFovDeg = 90.0f;
    }

private:
    void updateVectors() {
        const float yaw = radians(yawDeg);
        const float pitch = radians(pitchDeg);
        Vec3 f{
            std::cos(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)
        };
        front = normalize(f);
        right = normalize(cross(front, worldUp));
        up = normalize(cross(right, front));
    }
};

#endif
