#ifndef PROJECT11_CAMERA_H_INCLUDED
#define PROJECT11_CAMERA_H_INCLUDED

#include "transformations.h"

#include <cmath>

// -----------------------------------------------------------------------------
// Vector basico para camara.
// -----------------------------------------------------------------------------
class Vec3 {
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 operator-(const Vec3& a, const Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 operator*(const Vec3& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}

inline Vec3 operator/(const Vec3& v, float s) {
    return {v.x / s, v.y / s, v.z / s};
}

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

inline float length(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

inline Vec3 normalize(const Vec3& v) {
    const float len = length(v);
    if (len < 1e-8f) return {0.0f, 0.0f, 0.0f};
    return v / len;
}

inline float radians(float deg) {
    return deg * (PI / 180.0f);
}

// -----------------------------------------------------------------------------
// LookAt estilo OpenGL.
// -----------------------------------------------------------------------------
inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 f = normalize(center - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 view = Mat4::identity();

    // IMPORTANTE:
    // Esta version usa el orden correcto para matrices column-major,
    // igual que tu proyecto anterior.
    view.m[0] = s.x;
    view.m[4] = s.y;
    view.m[8] = s.z;

    view.m[1] = u.x;
    view.m[5] = u.y;
    view.m[9] = u.z;

    view.m[2] = -f.x;
    view.m[6] = -f.y;
    view.m[10] = -f.z;

    view.m[12] = -dot(s, eye);
    view.m[13] = -dot(u, eye);
    view.m[14] = dot(f, eye);

    return view;
}

inline Mat4 gluLookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    return lookAt(eye, center, up);
}

// -----------------------------------------------------------------------------
// Perspective.
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
// Orthographic.
// -----------------------------------------------------------------------------
inline Mat4 orthographic(
    float left,
    float right,
    float bottom,
    float top,
    float zNear,
    float zFar
) {
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
// Camara libre tipo videojuego.
// Mouse: solo cambia direccion.
// W/S: avanza/retrocede usando front completo, incluyendo Y.
// A/D: movimiento lateral.
// Q/E: baja/sube verticalmente.
// -----------------------------------------------------------------------------
enum class CameraMove {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

class Camera {
public:
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

    Camera() {
        updateVectors();
    }

    explicit Camera(const Vec3& startPos)
        : position(startPos) {
        updateVectors();
    }

	Mat4 viewMatrix() const {
		return lookAt(position, position + front, up);
	}

	void move(CameraMove dir, float deltaTime) {
		const float velocity = moveSpeed * deltaTime;

		if (dir == CameraMove::Forward) {
			position = position + front * velocity;
		}

		if (dir == CameraMove::Backward) {
			position = position - front * velocity;
		}

		if (dir == CameraMove::Left) {
			position = position - right * velocity;
		}

		if (dir == CameraMove::Right) {
			position = position + right * velocity;
		}

		if (dir == CameraMove::Up) {
			position = position + worldUp * velocity;
		}

		if (dir == CameraMove::Down) {
			position = position - worldUp * velocity;
		}
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