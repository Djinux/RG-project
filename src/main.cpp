#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
}dirLight;


struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
}pointLight;

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
}spotLight;


struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

int flag = 1;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
//    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
//    Shader nmShader("resources/shaders/normal_mapping.vs", "resources/shaders/normal_mapping.fs");
    Shader pbShader("resources/shaders/bottle.vs", "resources/shaders/bottle.fs");
    Shader trashShader("resources/shaders/trash.vs", "resources/shaders/trash.fs");

    // load models
    // -----------
    Model ourModel("resources/objects/dusty_road/scene.gltf");
    ourModel.SetShaderTextureNamePrefix("material.");
    Model dumpster("resources/objects/dumpster/scene.gltf");
    dumpster.SetShaderTextureNamePrefix("material.");
    Model tree("resources/objects/oak/Oak.obj");
    tree.SetShaderTextureNamePrefix("material.");
    Model trashBag("resources/objects/trash_bag/scene.gltf");
    trashBag.SetShaderTextureNamePrefix("material.");
    Model streetLight("resources/objects/rusty_streetlight/Light Pole.obj");
    streetLight.SetShaderTextureNamePrefix("material.");
    Model plasticBottle("resources/objects/plastic_water_bottle/scene.gltf");
    streetLight.SetShaderTextureNamePrefix("material.");
    Model pile("resources/objects/pile/scene.gltf");
    pile.SetShaderTextureNamePrefix("material.");
    Model oilBarrel("resources/objects/oil_barrel/scene.gltf");
    oilBarrel.SetShaderTextureNamePrefix("material.");
    Model canister("resources/objects/canister/scene.gltf");
    canister.SetShaderTextureNamePrefix("material.");
    Model oldCan("resources/objects/old_coca_cola_can/scene.gltf");
    oldCan.SetShaderTextureNamePrefix("material.");

//    PointLight& pointLight = programState->pointLight;
//    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-0.59, 2.0, -1.1);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.941, 0.933, 0.612);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    dirLight.direction = glm::vec3(-0.2, -1.0, -0.3);
    dirLight.ambient = glm::vec3(0.05, 0.05, -0.05);
    dirLight.diffuse = glm::vec3(0.4, 0.4, 0.4);
    dirLight.specular = glm::vec3( 0.5, 0.5, 0.5);

    spotLight.position = programState->camera.Position;
    spotLight.direction = programState->camera.Front;
    spotLight.ambient = glm::vec3(0.0, 0.0, 0.0);
    spotLight.diffuse = glm::vec3(1.0, 1.0, 1.0);
    spotLight.ambient = glm::vec3(1.0, 1.0, 1.0);

    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;

    spotLight.cutOff = glm::cos(glm::radians(12.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(15.0f));

    
    
    programState->camera.Position = glm::vec3(1.0f);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms


        //Dusty Road
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.0025));    // it's a bit too big for nm scene, so scale it down
        trashShader.setMat4("model", model);
        ourModel.Draw(trashShader);



        // Dumpster
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.2, 0.0, 0.0)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.012));    // it's a bit too big for our scene, so scale it down
        trashShader.setMat4("model", model);
        dumpster.Draw(trashShader);


        // Oak Tree
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0, 0.0, -3.0)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.15));    // it's a bit too big for trash scene, so scale it down
        trashShader.setMat4("model", model);
        tree.Draw(trashShader);


        // Trash Bag

        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0, 0.21, 0.9)); // translate it down so it's at the center of the scene
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.3));    // it's a bit too big for trash scene, so scale it down
        trashShader.setMat4("model", model);
        trashBag.Draw(trashShader);

        // Streetlight
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.2, 1.2, -1.1)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.1));    // it's a bit too big for trash scene, so scale it down
        trashShader.setMat4("model", model);
        streetLight.Draw(trashShader);


        // Pile
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.5, -0.05, -1.7)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.7));    // it's a bit too big for trash scene, so scale it down
        trashShader.setMat4("model", model);
        pile.Draw(trashShader);


        // Oil Barrel
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.3, 0.01, 1.0)); // translate it down so it's at the center of the scene
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        //model = glm::scale(model, glm::vec3(0.1));    // it's a bit too big for trash scene, so scale it down
        trashShader.setMat4("model", model);
        oilBarrel.Draw(trashShader);


        // Canister
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.85, 1.75, -1.05)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.009));    // it's a bit too big for trash scene, so scale it down
        trashShader.setMat4("model", model);
        canister.Draw(trashShader);


        // Old CocaCola Can
        trashShader.use();
        trashShader.setInt("flag", flag);
        trashShader.setVec3("dirLight.direction", dirLight.direction);
        trashShader.setVec3("dirLight.ambient", dirLight.ambient);
        trashShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        trashShader.setVec3("dirLight.specular", dirLight.specular);

        trashShader.setVec3("pointLight.position", pointLight.position);
        trashShader.setVec3("pointLight.ambient", pointLight.ambient);
        trashShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        trashShader.setVec3("pointLight.specular", pointLight.specular);
        trashShader.setFloat("pointLight.constant", pointLight.constant);
        trashShader.setFloat("pointLight.linear", pointLight.linear);
        trashShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        trashShader.setVec3("spotLight.position", programState->camera.Position);
        trashShader.setVec3("spotLight.direction", programState->camera.Front);
        trashShader.setVec3("spotLight.ambient", spotLight.ambient);
        trashShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        trashShader.setVec3("spotLight.specular", spotLight.specular);
        trashShader.setFloat("spotLight.constant", spotLight.constant);
        trashShader.setFloat("spotLight.linear", spotLight.linear);
        trashShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        trashShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        trashShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        trashShader.setVec3("viewPos", programState->camera.Position);
        trashShader.setFloat("material.shininess", 32.0);
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.54, 0.08, -0.5)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.1));    // it's a bit too big for trash scene, so scale it down
        trashShader.setMat4("model", model);
        oldCan.Draw(trashShader);


        // Plastic Bottle
        glEnable(GL_BLEND);
        pbShader.use();
        pbShader.setInt("flag", flag);
        pbShader.setVec3("dirLight.direction", dirLight.direction);
        pbShader.setVec3("dirLight.ambient", dirLight.ambient);
        pbShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        pbShader.setVec3("dirLight.specular", dirLight.specular);

        pbShader.setVec3("pointLight.position", pointLight.position);
        pbShader.setVec3("pointLight.ambient", pointLight.ambient);
        pbShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        pbShader.setVec3("pointLight.specular", pointLight.specular);
        pbShader.setFloat("pointLight.constant", pointLight.constant);
        pbShader.setFloat("pointLight.linear", pointLight.linear);
        pbShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        pbShader.setVec3("spotLight.position", programState->camera.Position);
        pbShader.setVec3("spotLight.direction", programState->camera.Front);
        pbShader.setVec3("spotLight.ambient", spotLight.ambient);
        pbShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        pbShader.setVec3("spotLight.specular", spotLight.specular);
        pbShader.setFloat("spotLight.constant", spotLight.constant);
        pbShader.setFloat("spotLight.linear", spotLight.linear);
        pbShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        pbShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        pbShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        pbShader.setVec3("viewPos", programState->camera.Position);
        pbShader.setFloat("material.shininess", 32.0);
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        pbShader.setMat4("projection", projection);
        pbShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.25, 0.01, 0.0)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.01));    // it's a bit too big for pb scene, so scale it down
        pbShader.setMat4("model", model);
        plasticBottle.Draw(pbShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.4, 0.02, -0.5)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.01));    // it's a bit too big for our scene, so scale it down
        pbShader.setMat4("model", model);
        plasticBottle.Draw(pbShader);

        glDisable(GL_BLEND);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, 2*deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, 2*deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, 2*deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, 2*deltaTime);


    if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        flag = 1;
    if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        flag = 2;
    if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        flag = 3;



}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}
