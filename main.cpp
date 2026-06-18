#define GLAD_GL_IMPLEMENTATION
#include "camera.h"
#include "draw3D.h"
#include "objetos.h"
#include "Porsche.h"
#include "Sky.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#ifndef ASSET_DIR
#define ASSET_DIR "Models/"
#endif

static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
static float getFramebufferAspect(GLFWwindow* window);
static void processInput(GLFWwindow* window, Camera& camera, float deltaTime);
static void updateMouseCamera(GLFWwindow* window, Camera& camera);

int main() {
    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW.\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    const int windowWidth = 1920;
    const int windowHeight = 1080;

    GLFWwindow* window = glfwCreateWindow(
        windowWidth,
        windowHeight,
        "Final Grafica - Porsche",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "No se pudo crear la ventana.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "No se pudo inicializar GLAD.\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);

    const std::string modelsDir = ASSET_DIR;
    const std::string roadDir = modelsDir + "Road/";
    const std::string porscheDir = modelsDir + "Porsche/";

    Objetos::BloqueCiudad ciudad(roadDir);
    PorscheCar porsche(porscheDir);

    Sky sky(modelsDir, 151.0f);

    if (!porsche.loaded()) {
        std::cerr << "El Porsche no cargo completo. Revisa rutas de OBJ/MTL/texturas.\n";
    }

    Camera camera({0.0f, 5.0f, 12.0f});
    camera.yawDeg = -90.0f;
    camera.pitchDeg = -10.0f;
    camera.moveSpeed = 8.0f;
    camera.mouseSensitivity = 0.12f;
    camera.zoomFovDeg = 45.0f;
    camera.rotateFromMouse(0.0f, 0.0f);

    glfwSetWindowUserPointer(window, &camera.zoomFovDeg);

    float lastTime = static_cast<float>(glfwGetTime());
    const float carSpeed = 2.0f;
	float aniBloq1 = -51.0f;
	float aniBloq2 = -51.0f;
	float speedAnimabloq1 = 0.70f;
	float speedAnimabloq2 = 0.30f;
	float animationTime = 0.0f;
	bool hasLigths = false;
	bool animationPaused = false;
	bool spaceWasPressed = false;
		
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const float currentTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        updateMouseCamera(window, camera);
        processInput(window, camera, deltaTime);

        const bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spacePressed && !spaceWasPressed) {
            animationPaused = !animationPaused;
        }
        spaceWasPressed = spacePressed;

        if (!animationPaused) {
            animationTime += deltaTime;
            porsche.update(deltaTime, carSpeed);
        }

        clearLights();
        LightingEffects effects{};
        effects.globalAmbient = {0.20f, 0.20f, 0.22f, 0.0f};
        effects.fogEnabled = true;
        effects.fogColor = {0.32f, 0.33f, 0.44f, 1.0f};
        effects.fogStart = 2.0f;
        effects.fogEnd = 100.0f;
        setLightingEffects(effects);

        /*addDirectionalLight(
            {-0.35f, -1.0f, -0.25f},
            {0.90f, 0.92f, 1.00f, 1.0f},
            0.4f
        );*/

        addDirectionalLight(
            {-6.5f, -10.0f, -0.25f},
            {0.66f, 0.00f, 1.00f, 1.0f},
            1.2f
        );
        addDirectionalLight(
            {6.5f, -10.0f, -0.25f},
            {0.0f, 0.7937f, 0.8487f, 1.0f},
            1.2f
        );
/*
        addPointLight(
            {-6.0f, 5.0f, 5.5f},
            {0.66f, 0.00f, 1.00f, 1.0f},
            5.0f,
            20.0f
        );

        addPointLight(
            {6.0f, 5.0f, -5.5f},
            {0.22f, 1.00f, 0.08f, 1.0f},
            4.0f,
            20.0f
        );
*/
        setProjectionMatrix(perspective(camera.zoomFovDeg, getFramebufferAspect(window), 0.1f, 350.0f));
        setViewMatrix(camera.viewMatrix());

        glClearColor(0.12f, 0.13f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sky.draw(effects);

		if (!animationPaused) {
			aniBloq1 += speedAnimabloq1;
			if (aniBloq1 >= 85.0f) {
				aniBloq1 = -51.0f;
			}
			
			aniBloq2 += speedAnimabloq2;
			if (aniBloq2 >= 85.0f) {
				aniBloq2 = -51.0f;
			}
		}


			ciudad.setLayerOffsets(aniBloq1, aniBloq2);
			ciudad.draw(true, animationTime);

			if (aniBloq1 >= -50.0f) {
				ciudad.setLayerOffsets(aniBloq1 - 136.0f, aniBloq2 -136.0f);
				ciudad.draw(false, animationTime);
			}


        if (porsche.loaded()) {
            porsche.draw();
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    (void)xOffset;

    float* fov = static_cast<float*>(glfwGetWindowUserPointer(window));
    if (!fov) return;

    *fov -= static_cast<float>(yOffset) * 2.0f;
    if (*fov < 20.0f) *fov = 20.0f;
    if (*fov > 75.0f) *fov = 75.0f;
}

float getFramebufferAspect(GLFWwindow* window) {
    int width = 1;
    int height = 1;
    glfwGetFramebufferSize(window, &width, &height);

    if (width <= 0) width = 1;
    if (height <= 0) height = 1;

    return static_cast<float>(width) / static_cast<float>(height);
}

void processInput(GLFWwindow* window, Camera& camera, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.move(CameraMove::Forward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.move(CameraMove::Backward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.move(CameraMove::Left, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.move(CameraMove::Right, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.move(CameraMove::Forward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.move(CameraMove::Backward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.move(CameraMove::Left, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.move(CameraMove::Right, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.move(CameraMove::Down, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.move(CameraMove::Up, deltaTime);
}

void updateMouseCamera(GLFWwindow* window, Camera& camera) {
    static bool dragging = false;
    static double lastMouseX = 0.0;
    static double lastMouseY = 0.0;

    const bool leftMouseDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (!leftMouseDown) {
        dragging = false;
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (!dragging) {
        dragging = true;
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        return;
    }

    const float xOffset = static_cast<float>(mouseX - lastMouseX);
    const float yOffset = static_cast<float>(lastMouseY - mouseY);

    const Vec3 cameraPosition = camera.position;
    camera.rotateFromMouse(xOffset, yOffset);
    camera.position = cameraPosition;
    lastMouseX = mouseX;
    lastMouseY = mouseY;
}
