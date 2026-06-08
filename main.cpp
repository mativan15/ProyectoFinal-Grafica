#define GLAD_GL_IMPLEMENTATION
#include "camera.h"
#include "draw3D.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>

using namespace std;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static float framebuffer_aspect(GLFWwindow* window);
static void framebuffer_size(GLFWwindow* window, int& width, int& height);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    const unsigned int SCR_W = 900;
    const unsigned int SCR_H = 900;
    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Project_10 - Lighting example", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    clearLights();
    LightingEffects effects{};
    effects.globalAmbient = {0.12f, 0.13f, 0.16f, 1.0f};
    effects.fogEnabled = true;
    effects.fogColor = {0.12f, 0.13f, 0.14f, 1.0f};
    effects.fogStart = 5.0f;
    effects.fogEnd = 10.0f;
    setLightingEffects(effects);

    addDirectionalLight({-0.35f, -1.0f, -0.25f}, {0.70f, 0.78f, 1.00f, 1.0f}, 0.75f);
    addPointLight({-1.6f, 1.35f, 0.6f}, {1.00f, 0.16f, 0.82f, 1.0f}, 2.4f, 1.0f, 0.25f, 0.08f);
    addPointLight({1.8f, 1.0f, -1.2f}, {0.20f, 0.70f, 1.00f, 1.0f}, 1.3f, 1.0f, 0.18f, 0.06f);

    GenShape teapot("teapot.obj");
    if (!teapot.loaded()) {
        cerr << teapot.error() << "\n";
        glfwTerminate();
        return -1;
    }
    teapot.color = {0.78f, 0.78f, 0.74f, 1.0f};
    teapot.enableLighting(Materials::MetalPole());

    Cube floor(4.8f, 0.08f, 3.4f);
    floor.color = {0.36f, 0.37f, 0.40f, 1.0f};
    floor.enableLighting(Materials::WetRoad());

    Cube cube(0.58f);
    cube.color = {0.35f, 0.58f, 1.0f, 1.0f};
    cube.enableLighting(Materials::GlossyCar());

    Pyramid pyramid(0.85f, 0.95f);
    pyramid.color = {0.82f, 0.62f, 0.32f, 1.0f};
    pyramid.enableLighting(Materials::MatteBuilding());

    Sphere sphere(0.34f, 24, 32);
    sphere.color = {0.18f, 0.85f, 1.0f, 1.0f};
    sphere.enableLighting(Materials::NeonEmissive({0.18f, 0.85f, 1.0f, 1.0f}, 0.55f));

    GlowHalo pinkHalo(0.75f, 64, {1.0f, 0.12f, 0.82f, 0.42f}, {1.0f, 0.12f, 0.82f, 0.0f});

    float cameraYaw = 0.0f;
    float cameraPitch = 0.22f;
    const float cameraDistance = 4.35f;
    const Vec3 cameraTarget{0.0f, -0.10f, 0.0f};
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool draggingCamera = false;
    const float dragSensitivity = 0.008f;

    cout << "Loaded teapot.obj into GenShape with a small lighting demo scene.\n";
    cout << "Hold left mouse button and drag to orbit the camera.\n";

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        const bool leftMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (leftMouseDown) {
            if (draggingCamera) {
                cameraYaw += static_cast<float>(mouseX - lastMouseX) * dragSensitivity;
                cameraPitch += static_cast<float>(mouseY - lastMouseY) * dragSensitivity;
                if (cameraPitch > 1.25f) cameraPitch = 1.25f;
                if (cameraPitch < -1.25f) cameraPitch = -1.25f;
            }
            draggingCamera = true;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        } else {
            draggingCamera = false;
        }

        const float t = static_cast<float>(glfwGetTime());

        teapot.transform.matrix =
            Mat4::translate3D(0.0f, 0.05f, 0.0f) *
            Mat4::scale3D(0.62f, 0.62f, 0.62f);
        floor.transform.matrix = Mat4::translate3D(0.0f, -0.86f, 0.0f);
        cube.transform.matrix =
            Mat4::translate3D(-1.45f, -0.55f, -0.20f) *
            Mat4::rotateY(t * 0.85f) *
            Mat4::rotateX(0.35f);
        pyramid.transform.matrix =
            Mat4::translate3D(1.45f, -0.39f, -0.10f) *
            Mat4::rotateY(-t * 0.55f);
        sphere.transform.matrix =
            Mat4::translate3D(0.0f, -0.52f, -1.20f) *
            Mat4::rotateY(t * 0.45f);
        pinkHalo.transform.matrix =
            Mat4::translate3D(-1.6f, 1.35f, 0.6f) *
            Mat4::scale3D(0.75f, 0.75f, 0.75f);

        const float aspect = framebuffer_aspect(window);
        const Vec3 cameraEye{
            cameraTarget.x + cameraDistance * cos(cameraPitch) * sin(cameraYaw),
            cameraTarget.y + cameraDistance * sin(cameraPitch),
            cameraTarget.z + cameraDistance * cos(cameraPitch) * cos(cameraYaw)
        };
        setProjectionMatrix(perspective(45.0f, aspect, 0.1f, 100.0f));
        setViewMatrix(lookAt(cameraEye,
                             cameraTarget,
                             {0.0f, 1.0f, 0.0f}));

        glClearColor(0.12f, 0.13f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        floor.draw();
        cube.draw();
        pyramid.draw();
        sphere.draw();
        teapot.draw();

        enableAdditiveBlending();
        glDepthMask(GL_FALSE);
        pinkHalo.draw();
        glDepthMask(GL_TRUE);
        disableBlending();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

float framebuffer_aspect(GLFWwindow* window) {
    int width = 1;
    int height = 1;
    framebuffer_size(window, width, height);
    if (height <= 0) return 1.0f;
    return static_cast<float>(width) / static_cast<float>(height);
}

void framebuffer_size(GLFWwindow* window, int& width, int& height) {
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
}
