#define GLAD_GL_IMPLEMENTATION
#include "draw3D.h"
#include "objetos.h"
#include "Porsche.h"
#include "Sky.h"
#include "CameraSystem.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#ifndef ASSET_DIR
#define ASSET_DIR "Models/"
#endif

static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
static float getFramebufferAspect(GLFWwindow* window);

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

    CameraSystem cameras{};
    glfwSetWindowUserPointer(window, &cameras);

    float lastTime = static_cast<float>(glfwGetTime());
    const float normalCarSpeed   = 2.0f;
	const float maxSpeed1 = 136.0f / 6.0f;
	const float maxSpeed2 = maxSpeed1 * (3.0f / 7.0f);
	float aniBloq1 = -51.0f;
	float aniBloq2 = -51.0f;
	float speedMultiplier = 1.0f;
	float animationTime   = 0.0f;
	bool hasLigths = false;
	bool animationPaused = false;
	bool spaceWasPressed = false;
		
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const float currentTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window, cameras, deltaTime);

        const bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        if (spacePressed && !spaceWasPressed) {
            animationPaused = !animationPaused;
        }

        spaceWasPressed = spacePressed;

        if (!animationPaused) {
            animationTime += deltaTime;
            float cycleTime = std::fmod(animationTime, 10.0f);

            // Semaforo en z=15: world_Z = aniBloq1-15, camara en Z=12
            // distancia = 12-(aniBloq1-15) = 27-aniBloq1
            bool  lightIsGreen      = (cycleTime < 5.0f);
            float trafficLightAhead = 27.0f - aniBloq1;

            bool shouldBrake = !lightIsGreen
                             && (trafficLightAhead > 0.0f)
                             && (trafficLightAhead < 15.0f);

            float targetMultiplier = shouldBrake ? 0.0f : 1.0f;
            float lerpRate = (targetMultiplier > speedMultiplier) ? 1.5f : 4.5f;
            speedMultiplier += (targetMultiplier - speedMultiplier) * lerpRate * deltaTime;
            speedMultiplier = std::max(0.0f, std::min(1.0f, speedMultiplier));

            porsche.update(deltaTime, normalCarSpeed * speedMultiplier);
        }

        clearLights();

        LightingEffects effects{};
        effects.globalAmbient = {0.20f, 0.20f, 0.22f, 0.0f};
        effects.fogEnabled = true;
        effects.fogColor = {0.32f, 0.33f, 0.44f, 1.0f};
        effects.fogStart = 2.0f;
        effects.fogEnd = 100.0f;

        setLightingEffects(effects);

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

        setProjectionMatrix(
            perspective(
                activeFov(cameras),
                getFramebufferAspect(window),
                0.1f,
                350.0f
            )
        );

        setViewMatrix(activeViewMatrix(cameras));

        glClearColor(0.12f, 0.13f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sky.draw(effects);

		if (!animationPaused) {
			aniBloq1 += maxSpeed1 * speedMultiplier * deltaTime;
			if (aniBloq1 >= 85.0f) aniBloq1 = -51.0f;
			aniBloq2 += maxSpeed2 * speedMultiplier * deltaTime;
			if (aniBloq2 >= 85.0f) aniBloq2 = -51.0f;
		}

			ciudad.setLayerOffsets(aniBloq1, aniBloq2);
			ciudad.draw(true, animationTime, true);

			if (aniBloq1 >= -50.0f) {
				ciudad.setLayerOffsets(aniBloq1 - 136.0f, aniBloq2 - 136.0f);
				ciudad.draw(false, animationTime, false);
			}

        if (aniBloq1 >= -50.0f) {
            ciudad.setLayerOffsets(aniBloq1 - 136.0f, aniBloq2 - 136.0f);
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

static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

static float getFramebufferAspect(GLFWwindow* window) {
    int width = 1;
    int height = 1;

    glfwGetFramebufferSize(window, &width, &height);

    if (width <= 0) width = 1;
    if (height <= 0) height = 1;

    return static_cast<float>(width) / static_cast<float>(height);
}