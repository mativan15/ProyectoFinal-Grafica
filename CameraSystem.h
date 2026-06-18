#ifndef PROJECT11_CAMERA_SYSTEM_H_INCLUDED
#define PROJECT11_CAMERA_SYSTEM_H_INCLUDED

#include "camera.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

// =========================================================
// Sistema de camaras
// ---------------------------------------------------------
// 1 = Camara general: atras/arriba del carro, con giro limitado.
// 2 = Camara conductor: dentro del carro, con giro limitado.
// 3 = Camara superior: vista aerea estatica.
// 4 = Camara lateral izquierda estatica.
// 5 = Camara lateral derecha estatica.
// L = Camara libre: tipo videojuego/Minecraft con mouse + WASD/QE.
// =========================================================

enum class ViewMode {
    General,
    Driver,
    Top,
    LeftSide,
    RightSide,
    Free
};

struct CameraSystem {
    ViewMode mode = ViewMode::General;

    Camera freeCamera{Vec3{0.0f, 5.0f, 12.0f}};

    float generalYawDeg = 0.0f;
    float driverYawDeg = 0.0f;

    float generalPitchDeg = 0.0f;
    float driverPitchDeg = 0.0f;

    float zoomFovDeg = 45.0f;

    bool key1WasPressed = false;
    bool key2WasPressed = false;
    bool key3WasPressed = false;
    bool key4WasPressed = false;
    bool key5WasPressed = false;
    bool keyLWasPressed = false;

    bool limitedDragging = false;
    double limitedLastX = 0.0;
    double limitedLastY = 0.0;

    bool freeFirstMouse = true;
    double freeLastX = 0.0;
    double freeLastY = 0.0;

    CameraSystem() {
        freeCamera.yawDeg = -90.0f;
        freeCamera.pitchDeg = -10.0f;
        freeCamera.moveSpeed = 10.0f;
        freeCamera.mouseSensitivity = 0.10f;
        freeCamera.zoomFovDeg = zoomFovDeg;
        freeCamera.rotateFromMouse(0.0f, 0.0f);
    }
};

inline bool pressedOnce(GLFWwindow* window, int key, bool& wasPressed) {
    const bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    const bool result = pressed && !wasPressed;
    wasPressed = pressed;
    return result;
}

inline void setCameraMode(GLFWwindow* window, CameraSystem& cameras, ViewMode mode) {
    cameras.mode = mode;
    cameras.limitedDragging = false;
    cameras.freeFirstMouse = true;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (mode == ViewMode::Free) {
        int width = 1;
        int height = 1;
        glfwGetWindowSize(window, &width, &height);

        const double centerX = width / 2.0;
        const double centerY = height / 2.0;

        glfwSetCursorPos(window, centerX, centerY);

        cameras.freeLastX = centerX;
        cameras.freeLastY = centerY;
    }
}

inline Vec3 rotateAroundY(const Vec3& v, float degrees) {
    const float a = radians(degrees);
    const float c = std::cos(a);
    const float s = std::sin(a);

    return {
        v.x * c + v.z * s,
        v.y,
        -v.x * s + v.z * c
    };
}

inline Vec3 directionWithYawPitch(const Vec3& baseFront, float yawOffsetDeg, float pitchOffsetDeg) {
    const Vec3 base = normalize(baseFront);

    const float baseYawDeg = std::atan2(base.z, base.x) * 180.0f / PI;
    const float basePitchDeg = std::asin(std::clamp(base.y, -1.0f, 1.0f)) * 180.0f / PI;

    const float finalYawDeg = baseYawDeg + yawOffsetDeg;
    const float finalPitchDeg = std::clamp(basePitchDeg + pitchOffsetDeg, -89.0f, 89.0f);

    const float yaw = radians(finalYawDeg);
    const float pitch = radians(finalPitchDeg);

    return normalize({
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch)
    });
}

inline Mat4 activeViewMatrix(const CameraSystem& cameras) {
    switch (cameras.mode) {
        case ViewMode::General: {
            const Vec3 eye{-0.435543f, 1.65087f, 12.1157f};
            const Vec3 baseFront{0.000829087f, 0.0165797f, -0.999862f};
            const Vec3 front = directionWithYawPitch(
                baseFront,
                cameras.generalYawDeg,
                cameras.generalPitchDeg
            );

            return lookAt(eye, eye + front * 18.0f, {0.0f, 1.0f, 0.0f});
        }

        case ViewMode::Driver: {
            const Vec3 eye{-0.465579f, 1.14266f, 3.46313f};
            const Vec3 baseFront{0.0122139f, 0.01658f, -0.999788f};
            const Vec3 front = directionWithYawPitch(
                baseFront,
                cameras.driverYawDeg,
                cameras.driverPitchDeg
            );

            return lookAt(eye, eye + front * 12.0f, {0.0f, 1.0f, 0.0f});
        }

        case ViewMode::Top: {
            return lookAt(
                {-0.568411f, 25.5725f, 7.09092f},
                {-0.568411f + 0.000725901f,
                 25.5725f - 0.985109f,
                 7.09092f - 0.171927f},
                {0.0f, 0.0f, -1.0f}
            );
        }

        case ViewMode::LeftSide: {
            return lookAt(
                {-4.6538f, 1.79433f, 9.17953f},
                {-4.6538f + 0.509939f,
                 1.79433f - 0.0932393f,
                 9.17953f - 0.855143f},
                {0.0f, 1.0f, 0.0f}
            );
        }

        case ViewMode::RightSide: {
            return lookAt(
                {3.77394f, 1.91538f, 8.96119f},
                {3.77394f - 0.477241f,
                 1.91538f - 0.13658f,
                 8.96119f - 0.868094f},
                {0.0f, 1.0f, 0.0f}
            );
        }

        case ViewMode::Free:
        default:
            return cameras.freeCamera.viewMatrix();
    }
}

inline float activeFov(const CameraSystem& cameras) {
    if (cameras.mode == ViewMode::Driver) {
        return 60.0f;
    }

    if (cameras.mode == ViewMode::Top) {
        return 48.0f;
    }

    if (cameras.mode == ViewMode::LeftSide || cameras.mode == ViewMode::RightSide) {
        return 42.0f;
    }

    return cameras.zoomFovDeg;
}

inline void updateLimitedMouseCamera(GLFWwindow* window, CameraSystem& cameras) {
    const bool leftMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (!leftMouseDown) {
        cameras.limitedDragging = false;
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (!cameras.limitedDragging) {
        cameras.limitedDragging = true;
        cameras.limitedLastX = mouseX;
        cameras.limitedLastY = mouseY;
        return;
    }

    const float xOffset = static_cast<float>(mouseX - cameras.limitedLastX);
    const float yawDelta = xOffset * 0.08f;

    if (cameras.mode == ViewMode::General) {
        cameras.generalYawDeg = std::clamp(
            cameras.generalYawDeg + yawDelta,
            -30.0f,
            30.0f
        );
    } else if (cameras.mode == ViewMode::Driver) {
        cameras.driverYawDeg = std::clamp(
            cameras.driverYawDeg + yawDelta,
            -40.0f,
            40.0f
        );
    }

    cameras.limitedLastX = mouseX;
    cameras.limitedLastY = mouseY;
}

inline void updateFreeMouseCamera(GLFWwindow* window, CameraSystem& cameras) {
    int width = 1;
    int height = 1;
    glfwGetWindowSize(window, &width, &height);

    const double centerX = width / 2.0;
    const double centerY = height / 2.0;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (cameras.freeFirstMouse) {
        cameras.freeFirstMouse = false;
        glfwSetCursorPos(window, centerX, centerY);
        cameras.freeLastX = centerX;
        cameras.freeLastY = centerY;
        return;
    }

    float xOffset = static_cast<float>(mouseX - centerX);
    float yOffset = static_cast<float>(centerY - mouseY);

    const float deadZone = 2.0f;

    if (std::abs(xOffset) < deadZone) {
        xOffset = 0.0f;
    }

    if (std::abs(yOffset) < deadZone) {
        yOffset = 0.0f;
    }

    if (xOffset == 0.0f && yOffset == 0.0f) {
        glfwSetCursorPos(window, centerX, centerY);
        cameras.freeLastX = centerX;
        cameras.freeLastY = centerY;
        return;
    }

    cameras.freeCamera.rotateFromMouse(xOffset, yOffset);

    glfwSetCursorPos(window, centerX, centerY);

    cameras.freeLastX = centerX;
    cameras.freeLastY = centerY;
}

inline void processInput(GLFWwindow* window, CameraSystem& cameras, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (pressedOnce(window, GLFW_KEY_1, cameras.key1WasPressed)) {
        setCameraMode(window, cameras, ViewMode::General);
    }

    if (pressedOnce(window, GLFW_KEY_2, cameras.key2WasPressed)) {
        setCameraMode(window, cameras, ViewMode::Driver);
    }

    if (pressedOnce(window, GLFW_KEY_3, cameras.key3WasPressed)) {
        setCameraMode(window, cameras, ViewMode::Top);
    }

    if (pressedOnce(window, GLFW_KEY_4, cameras.key4WasPressed)) {
        setCameraMode(window, cameras, ViewMode::LeftSide);
    }

    if (pressedOnce(window, GLFW_KEY_5, cameras.key5WasPressed)) {
        setCameraMode(window, cameras, ViewMode::RightSide);
    }

    if (pressedOnce(window, GLFW_KEY_L, cameras.keyLWasPressed)) {
        setCameraMode(window, cameras, ViewMode::Free);
    }

    if (cameras.mode == ViewMode::Free) {
        updateFreeMouseCamera(window, cameras);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameras.freeCamera.move(CameraMove::Forward, deltaTime);
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameras.freeCamera.move(CameraMove::Backward, deltaTime);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameras.freeCamera.move(CameraMove::Left, deltaTime);
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameras.freeCamera.move(CameraMove::Right, deltaTime);
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            cameras.freeCamera.move(CameraMove::Down, deltaTime);
        }

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            cameras.freeCamera.move(CameraMove::Up, deltaTime);
        }

        return;
    }

    if (cameras.mode == ViewMode::General || cameras.mode == ViewMode::Driver) {
        const float yawSpeed = 65.0f;
        const float pitchSpeed = 45.0f;

        float deltaYaw = 0.0f;
        float deltaPitch = 0.0f;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            deltaYaw -= yawSpeed * deltaTime;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            deltaYaw += yawSpeed * deltaTime;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            deltaPitch += pitchSpeed * deltaTime;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            deltaPitch -= pitchSpeed * deltaTime;
        }

        if (cameras.mode == ViewMode::General) {
            cameras.generalYawDeg = std::clamp(
                cameras.generalYawDeg + deltaYaw,
                -30.0f,
                30.0f
            );

            cameras.generalPitchDeg = std::clamp(
                cameras.generalPitchDeg + deltaPitch,
                -12.0f,
                12.0f
            );
        } else {
            cameras.driverYawDeg = std::clamp(
                cameras.driverYawDeg + deltaYaw,
                -40.0f,
                40.0f
            );

            cameras.driverPitchDeg = std::clamp(
                cameras.driverPitchDeg + deltaPitch,
                -30.0f,
                10.0f
            );
        }

        updateLimitedMouseCamera(window, cameras);
    }
}

inline void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    (void)xOffset;

    CameraSystem* cameras = static_cast<CameraSystem*>(glfwGetWindowUserPointer(window));
    if (!cameras) return;

    cameras->zoomFovDeg -= static_cast<float>(yOffset) * 2.0f;
    cameras->zoomFovDeg = std::clamp(cameras->zoomFovDeg, 20.0f, 75.0f);
    cameras->freeCamera.zoomFovDeg = cameras->zoomFovDeg;
}

#endif
