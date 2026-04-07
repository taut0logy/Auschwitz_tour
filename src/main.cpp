#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>

#include "Camera.h"
#include "Shader.h"
#include "Scene.h"

// ---- Window dimensions ----
int SCR_WIDTH  = 1920;
int SCR_HEIGHT = 1080;

// ---- Global state ----
Camera camera;
Scene  scene;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cursorLocked = true;

// ---- Callbacks ----
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    scene.windowWidth = width;
    scene.windowHeight = height;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!cursorLocked) return;
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll((float)yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    }
    if (key == GLFW_KEY_T) {
        scene.dayNight.cycleSpeed();
        std::cout << "Time speed: " << scene.dayNight.getSpeedLabel() << std::endl;
    }
    if (key == GLFW_KEY_H) {
        scene.hud.toggle();
    }
    if (key == GLFW_KEY_C) {
        cursorLocked = !cursorLocked;
        glfwSetInputMode(window, GLFW_CURSOR,
            cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    if (key == GLFW_KEY_F) {
        // Toggle fullscreen (basic implementation)
        static bool isFullscreen = false;
        if (!isFullscreen) {
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0,
                mode->width, mode->height, mode->refreshRate);
            isFullscreen = true;
        } else {
            glfwSetWindowMonitor(window, nullptr, 100, 100, 1920, 1080, 0);
            isFullscreen = false;
        }
    }
}

void processInput(GLFWwindow* window) {
    bool sprint = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(CAM_FORWARD, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(CAM_BACKWARD, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(CAM_LEFT, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(CAM_RIGHT, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.processKeyboard(CAM_UP, deltaTime, sprint);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.processKeyboard(CAM_DOWN, deltaTime, sprint);
}

int main() {
    // ---- GLFW init ----
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Auschwitz I - Educational 3D Reconstruction", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ---- GLAD init ----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // ---- OpenGL state ----
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);

    // ---- Load shaders ----
    Shader phongShader("shaders/phong.vert", "shaders/phong.frag");
    Shader alphaShader("shaders/alpha.vert", "shaders/alpha.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
    Shader unlitShader("shaders/unlit.vert", "shaders/unlit.frag");
    Shader particleShader("shaders/particle.vert", "shaders/particle.frag");
    Shader hudShader("shaders/hud.vert", "shaders/hud.frag");

    // ---- Init scene ----
    scene.windowWidth = SCR_WIDTH;
    scene.windowHeight = SCR_HEIGHT;
    scene.hud.hudShader = &hudShader;
    scene.init();

    std::cout << "=== Auschwitz I 3D Educational Reconstruction ===\n";
    std::cout << "Controls:\n";
    std::cout << "  WASD  - Move      Shift - Sprint\n";
    std::cout << "  Space - Up        Ctrl  - Down\n";
    std::cout << "  Mouse - Look      Scroll- Zoom\n";
    std::cout << "  T     - Time speed (pause/1x/8x/30x)\n";
    std::cout << "  H     - Toggle HUD\n";
    std::cout << "  C     - Toggle cursor lock\n";
    std::cout << "  F     - Toggle fullscreen\n";
    std::cout << "  ESC   - Quit\n";

    // ---- Main render loop ----
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Cap delta time to prevent physics explosion
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        processInput(window);
        scene.update(deltaTime, camera);

        // View / projection
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 800.0f);

        // ========== PASS 1: Clear + Skybox ==========
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene.renderSkybox(skyboxShader, view, projection);

        // ========== PASS 2: Stars ==========
        scene.renderStars(unlitShader, view, projection);

        // ========== PASS 3: Opaque ==========
        scene.renderOpaque(phongShader, camera, view, projection);

        // ========== PASS 4: Celestial bodies ==========
        scene.renderCelestial(unlitShader, view, projection);

        // ========== PASS 5: Alpha ==========
        scene.renderAlpha(alphaShader, camera, view, projection);

        // ========== PASS 6: Particles ==========
        scene.renderParticles(particleShader, camera, view, projection);

        // ========== PASS 7: HUD ==========
        scene.renderHUD(SCR_WIDTH, SCR_HEIGHT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    scene.cleanup();
    glfwTerminate();
    return 0;
}
