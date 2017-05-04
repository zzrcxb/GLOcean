//     _   __ _____    
//    / | / /\___  \   
//   /  |/ /    /  /   
//  / /|  /    /  /__  
// /_/ |_/     \_____\ 
//                     


// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STD
#include <iostream>
#include <vector>

// GL includes
#include "Shader.h"
#include "Camera.h"
#include "ocean.h"
#include "skybox.h"


// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();
inline void fInitGL(GLFWwindow* &window);

GLuint screenWidth = 1920, screenHeight = 1080;
// Camera
Camera camera(glm::vec3(0.0f, 0.003f, 0.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
glm::vec3 lightPos(0.0f, 10.0f, -20.0f);

using namespace std;

int main() {
    GLFWwindow* window;
    fInitGL(window);

    vector<const GLchar*> faces;
    faces.push_back("skybox/right.jpg");
    faces.push_back("skybox/left.jpg");
    faces.push_back("skybox/top.jpg");
    faces.push_back("skybox/bottom.jpg");
    faces.push_back("skybox/back.jpg");
    faces.push_back("skybox/front.jpg");
    Shader skyshaders("shaders/skybox.vs", "shaders/skybox.frag");
    SkyBox sbox(faces, skyshaders);

    Shader shader("shaders/vertex.txt", "shaders/fragment.txt");
    cOcean ocean(128, 0.00005f, glm::vec2(32.0f, 32.0f), 2.0f, false);
    ocean.bind(shader);

    float t = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        t += 0.003f;
        // Set frame time
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check and call events
        glfwPollEvents();
        Do_Movement();
        cout << "\r" << 1 / deltaTime << " fps";

        // Clear the colorbuffer
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 projection = glm::perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.001f, 100.0f);

        glm::mat4 model;
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        glm::mat3 tmp1 = glm::mat3(camera.GetViewMatrix());
        glm::mat4 tmp2 = camera.GetViewMatrix();
        sbox.draw(tmp2, projection, model);

        view = camera.GetViewMatrix();
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        //model = glm::scale(model, glm::vec3(0.2f));
        ocean.render(t, lightPos, camera.Position, projection, view, model, false, camera.Position, sbox.getTexture());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}


inline void fInitGL(GLFWwindow* &window) {
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    window = glfwCreateWindow(screenWidth, screenHeight, "Grating", nullptr, nullptr); // Windowed
    glfwMakeContextCurrent(window);

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Options
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW to setup the OpenGL Function pointers
    glewExperimental = GL_TRUE;
    glewInit();

    // Define the viewport dimensions
    glViewport(0, 0, screenWidth, screenHeight);

    // Setup some OpenGL options
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}


#pragma region "User input"

// Moves/alters the camera positions based on user input
void Do_Movement() {
    // Camera controls
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

#pragma endregion