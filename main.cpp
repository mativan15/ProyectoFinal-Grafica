#define GLAD_GL_IMPLEMENTATION
#include "camera.h"
#include "draw3D.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>


using namespace std;
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static float framebuffer_aspect(GLFWwindow* window);
static void framebuffer_size(GLFWwindow* window, int& width, int& height);
static void update_window_title(GLFWwindow* window, bool perspectiveMode);


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
    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Project_10 - Projection demo", nullptr, nullptr);
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

    vector<Color> cubeFaces = {
        {0.90f, 0.12f, 0.12f, 1.0f}, // back
        {0.10f, 0.48f, 0.95f, 1.0f}, // front
        {0.95f, 0.75f, 0.10f, 1.0f}, // bottom
        {0.10f, 0.75f, 0.35f, 1.0f}, // top
        {0.95f, 0.95f, 0.95f, 1.0f}, // left
        {0.95f, 0.45f, 0.10f, 1.0f}  // right
    };

    ShapeGroup scene;

    for (int i = -6; i <= 2; ++i) {
        const float z = static_cast<float>(i);
        auto line = make_unique<GenShape>(
            vector<float>{-3.5f, 0.0f, z, 3.5f, 0.0f, z},
            3, vector<unsigned int>{}, GL_LINES);
        line->color = {0.42f, 0.42f, 0.42f, 1.0f};
        line->lineWidth = 1.0f;
        scene.addOwned(std::move(line));
    }

    for (int i = -3; i <= 3; ++i) {
        const float x = static_cast<float>(i);
        auto line = make_unique<GenShape>(
            vector<float>{x, 0.0f, 2.0f, x, 0.0f, -6.0f},
            3, vector<unsigned int>{}, GL_LINES);
        line->color = {0.42f, 0.42f, 0.42f, 1.0f};
        line->lineWidth = 1.0f;
        scene.addOwned(std::move(line));
    }

    auto nearCube = make_unique<Cube>(0.85f, cubeFaces);
    nearCube->transform.matrix = trs3D(-2.0f, 0.48f, 0.0f,
                                       0.35f, 0.55f, 0.0f,
                                       1.0f, 1.0f, 1.0f);
    scene.addOwned(std::move(nearCube));

    auto midCube = make_unique<Cube>(0.85f, cubeFaces);
    midCube->transform.matrix = trs3D(0.0f, 0.48f, -2.5f,
                                      0.35f, 0.55f, 0.0f,
                                      1.0f, 1.0f, 1.0f);
    scene.addOwned(std::move(midCube));

    auto farCube = make_unique<Cube>(0.85f, cubeFaces);
    farCube->transform.matrix = trs3D(2.0f, 0.48f, -5.0f,
                                      0.35f, 0.55f, 0.0f,
                                      1.0f, 1.0f, 1.0f);
    scene.addOwned(std::move(farCube));

    auto pyramid = make_unique<Pyramid>(0.85f, 1.1f);
    pyramid->color = {0.80f, 0.20f, 0.85f, 1.0f};
    pyramid->transform.matrix = trs3D(-2.9f, 0.55f, -4.5f,
                                      0.0f, 0.6f, 0.0f,
                                      1.0f, 1.0f, 1.0f);
    scene.addOwned(std::move(pyramid));

    auto sphere = make_unique<Sphere>(0.45f, 18, 28);
    sphere->color = {0.10f, 0.80f, 0.85f, 1.0f};
    sphere->transform.matrix = trs3D(2.9f, 0.5f, -0.8f,
                                     0.0f, 0.0f, 0.0f,
                                     1.0f, 1.0f, 1.0f);
    scene.addOwned(std::move(sphere));

    ShapeGroup hud;

    auto hudPanel = make_unique<Rectangle>(310.0f, 86.0f);
    hudPanel->color = {0.05f, 0.06f, 0.07f, 1.0f};
    hudPanel->transform.matrix = Mat4::translate2D(173.0f, 58.0f);
    hud.addOwned(std::move(hudPanel));

    auto pixelSquare = make_unique<Rectangle>(46.0f, 46.0f);
    pixelSquare->color = {0.95f, 0.75f, 0.10f, 1.0f};
    pixelSquare->transform.matrix = Mat4::translate2D(48.0f, 58.0f);
    hud.addOwned(std::move(pixelSquare));

    auto pixelCircle = make_unique<Circle>(22.0f, 32);
    pixelCircle->color = {0.10f, 0.80f, 0.85f, 1.0f};
    pixelCircle->transform.matrix = Mat4::translate2D(112.0f, 58.0f);
    hud.addOwned(std::move(pixelCircle));

    auto pixelLine = make_unique<Line>(148.0f, 39.0f, 288.0f, 39.0f, 5.0f);
    pixelLine->color = {0.95f, 0.95f, 0.95f, 1.0f};
    hud.addOwned(std::move(pixelLine));

    auto perspectiveBadge = make_unique<Rectangle>(66.0f, 20.0f);
    Rectangle* perspectiveBadgePtr = perspectiveBadge.get();
    perspectiveBadge->transform.matrix = Mat4::translate2D(181.0f, 66.0f);
    hud.addOwned(std::move(perspectiveBadge));

    auto orthographicBadge = make_unique<Rectangle>(66.0f, 20.0f);
    Rectangle* orthographicBadgePtr = orthographicBadge.get();
    orthographicBadge->transform.matrix = Mat4::translate2D(255.0f, 66.0f);
    hud.addOwned(std::move(orthographicBadge));

    bool perspectiveMode = true;
    bool spaceWasDown = false;
    bool pWasDown = false;
    bool oWasDown = false;
    update_window_title(window, perspectiveMode);
    cout << "Projection demo controls: SPACE toggles, P = perspective, O = orthographic, ESC = close\n";
    cout << "The top-left overlay uses ortho2D(0, width, height, 0) and stays in screen pixels.\n";

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        const bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        const bool pDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        const bool oDown = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;

        if (spaceDown && !spaceWasDown) {
            perspectiveMode = !perspectiveMode;
            update_window_title(window, perspectiveMode);
        }
        if (pDown && !pWasDown) {
            perspectiveMode = true;
            update_window_title(window, perspectiveMode);
        }
        if (oDown && !oWasDown) {
            perspectiveMode = false;
            update_window_title(window, perspectiveMode);
        }

        spaceWasDown = spaceDown;
        pWasDown = pDown;
        oWasDown = oDown;

        const float aspect = framebuffer_aspect(window);
        setViewMatrix(lookAt({0.0f, 2.1f, 6.0f},
                             {0.0f, 0.35f, -2.4f},
                             {0.0f, 1.0f, 0.0f}));
        if (perspectiveMode) {
            setProjectionMatrix(perspective(45.0f, aspect, 0.1f, 100.0f));
        } else {
            const float halfHeight = 3.0f;
            setProjectionMatrix(orthographic(-halfHeight * aspect, halfHeight * aspect,
                                             -halfHeight, halfHeight,
                                             0.1f, 100.0f));
        }

        glClearColor(0.12f, 0.13f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene.draw();

        perspectiveBadgePtr->color = perspectiveMode
            ? Color{0.10f, 0.75f, 0.35f, 1.0f}
            : Color{0.24f, 0.24f, 0.24f, 1.0f};
        orthographicBadgePtr->color = perspectiveMode
            ? Color{0.24f, 0.24f, 0.24f, 1.0f}
            : Color{0.10f, 0.75f, 0.35f, 1.0f};

        int fbWidth = 1;
        int fbHeight = 1;
        framebuffer_size(window, fbWidth, fbHeight);
        glDisable(GL_DEPTH_TEST);
        setViewMatrix(Mat4::identity());
        setProjectionMatrix(ortho2D(0.0f, static_cast<float>(fbWidth),
                                    static_cast<float>(fbHeight), 0.0f));
        hud.draw();
        glEnable(GL_DEPTH_TEST);

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

void update_window_title(GLFWwindow* window, bool perspectiveMode) {
    glfwSetWindowTitle(
        window,
        perspectiveMode
            ? "Project_10 - Perspective 3D + ortho2D overlay"
            : "Project_10 - Orthographic 3D + ortho2D overlay"
    );
}
