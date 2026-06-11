#define GLAD_GL_IMPLEMENTATION
#include "camera.h"
#include "draw3D.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <string>

using namespace std;

#ifndef ASSET_DIR
#define ASSET_DIR "Models/Porsche/"
#endif

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static float framebuffer_aspect(GLFWwindow* window);
static void framebuffer_size(GLFWwindow* window, int& width, int& height);

static Material glossyCarMaterial() {
    Material material{};
    material.ambient = {0.04f, 0.04f, 0.05f, 1.0f};
    material.diffuse = {0.85f, 0.05f, 0.08f, 1.0f};
    material.specular = {1.0f, 0.95f, 0.85f, 1.0f};
    material.shininess = 96.0f;
    return material;
}

static Material wetRoadMaterial() {
    Material material{};
    material.ambient = {0.03f, 0.03f, 0.035f, 1.0f};
    material.diffuse = {0.10f, 0.11f, 0.12f, 1.0f};
    material.specular = {0.80f, 0.82f, 0.86f, 1.0f};
    material.shininess = 72.0f;
    return material;
}

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

    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Project_10 - Car", nullptr, nullptr);
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

    const string modelDir = ASSET_DIR;
    cout << "Asset path: " << modelDir << '\n';

    GenShape carBody(modelDir + "Body_Car.obj");
    if (!carBody.loaded()) {
        cerr << "Error loading Body_Car.obj: " << carBody.error() << '\n';
        glfwTerminate();
        return -1;
    }

    GenShape wheel(modelDir + "Wheel.obj");
    if (!wheel.loaded()) {
        cerr << "Error loading Wheel.obj: " << wheel.error() << '\n';
        glfwTerminate();
        return -1;
    }

    carBody.color = {0.35f, 0.04f, 0.03f, 1.0f};
    carBody.enableLighting(glossyCarMaterial());

    TextureOptions wheelTexOptions{};
    wheelTexOptions.flipVertically = true;
    wheelTexOptions.srgb = true;

    Texture2D wheelTexture = loadTexture2D(
        modelDir + "Textures/Wheel_Final.png",
        wheelTexOptions
    );
    if (!wheelTexture.valid()) {
        cerr << "Error loading Wheel_Final.png\n";
    }

    Material wheelMaterial{};
    wheelMaterial.ambient = {0.85f, 0.85f, 0.85f, 1.0f};
    wheelMaterial.diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
    wheelMaterial.specular = {0.0f, 0.0f, 0.0f, 1.0f};
    wheelMaterial.emissive = {0.0f, 0.0f, 0.0f, 1.0f};
    wheelMaterial.shininess = 8.0f;
    wheelMaterial.opacity = 1.0f;
    wheelMaterial.diffuseMap = wheelTexture.id;

    wheel.color = {1.0f, 1.0f, 1.0f, 1.0f};
    wheel.enableLighting(wheelMaterial);

    Cube floor(30.0f, 0.08f, 24.0f);
    floor.color = {0.28f, 0.29f, 0.31f, 1.0f};
    floor.enableLighting(wetRoadMaterial());

    float cameraYaw = 0.0f;
    float cameraPitch = 0.38f;
    const float cameraDistance = 16.0f;
    const Vec3 cameraTarget{0.0f, 1.5f, 0.0f};

    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool draggingCamera = false;
    const float dragSensitivity = 0.008f;

    float wheelRotation = 0.0f;
    const float currentSpeed = 2.0f;
    float lastTime = static_cast<float>(glfwGetTime());

    cout << "Car model loaded.\n";
    cout << "Hold left mouse button and drag to orbit the camera.\n";
    cout << "ESC to close.\n";

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        const bool leftMouseDown =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

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

        const float time = static_cast<float>(glfwGetTime());
        const float deltaTime = time - lastTime;
        lastTime = time;
        wheelRotation += currentSpeed * deltaTime * 5.0f;

        clearLights();

        LightingEffects effects{};
        effects.globalAmbient = {0.20f, 0.20f, 0.22f, 1.0f};
        effects.fogEnabled = true;
        effects.fogColor = {0.12f, 0.13f, 0.14f, 1.0f};
        effects.fogStart = 18.0f;
        effects.fogEnd = 40.0f;
        setLightingEffects(effects);

        addDirectionalLight(
            {-0.35f, -1.0f, -0.25f},
            {0.90f, 0.92f, 1.00f, 1.0f},
            1.2f
        );
        addPointLight(
            {-6.0f, 5.0f, 5.5f},
            {1.00f, 0.82f, 0.45f, 1.0f},
            5.0f,
            14.0f
        );
        addPointLight(
            {6.0f, 5.0f, -5.5f},
            {0.35f, 0.65f, 1.00f, 1.0f},
            4.0f,
            14.0f
        );

        const float bodyScale = 8.0f;
        const float wheelScale = 1.0f;
        const float wheelX = 2.8f;
        const float wheelY = 1.0f;
        const float wheelZFront = 4.4f;
        const float wheelZRear = -4.4f;

        carBody.transform.matrix =
            Mat4::translate3D(0.0f, 2.25f, 0.0f) *
            Mat4::scale3D(bodyScale, bodyScale, bodyScale);
        floor.transform.matrix = Mat4::translate3D(0.0f, 0.0f, 0.0f);

        const float aspect = framebuffer_aspect(window);
        const Vec3 cameraEye{
            cameraTarget.x + cameraDistance * cos(cameraPitch) * sin(cameraYaw),
            cameraTarget.y + cameraDistance * sin(cameraPitch),
            cameraTarget.z + cameraDistance * cos(cameraPitch) * cos(cameraYaw)
        };

        setProjectionMatrix(perspective(45.0f, aspect, 0.1f, 100.0f));
        setViewMatrix(lookAt(cameraEye, cameraTarget, {0.0f, 1.0f, 0.0f}));

        glClearColor(0.12f, 0.13f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        floor.draw();
        carBody.draw();

        wheel.transform.matrix =
            Mat4::translate3D(-wheelX, wheelY, wheelZFront) *
            Mat4::rotateY(PI) *
            Mat4::rotateX(-wheelRotation) *
            Mat4::scale3D(wheelScale, wheelScale, wheelScale);
        wheel.draw();

        wheel.transform.matrix =
            Mat4::translate3D(wheelX, wheelY, wheelZFront) *
            Mat4::rotateX(wheelRotation) *
            Mat4::scale3D(wheelScale, wheelScale, wheelScale);
        wheel.draw();

        wheel.transform.matrix =
            Mat4::translate3D(-wheelX, wheelY, wheelZRear) *
            Mat4::rotateY(PI) *
            Mat4::rotateX(-wheelRotation) *
            Mat4::scale3D(wheelScale, wheelScale, wheelScale);
        wheel.draw();

        wheel.transform.matrix =
            Mat4::translate3D(wheelX, wheelY, wheelZRear) *
            Mat4::rotateX(wheelRotation) *
            Mat4::scale3D(wheelScale, wheelScale, wheelScale);
        wheel.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteTexture(wheelTexture);
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
