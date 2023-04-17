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

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(const char *path);

void renderPlank(unsigned int plankVAO, unsigned int plankVBO);


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

// flag for shaders to use to determine which light to use
int flag = 1;
bool flag1 = true;
bool flag2 = false;
bool flag3 = false;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
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

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);


    // build and compile shaders
    // Plastic water bottle has its own shader cause of the blending
    Shader pbShader("resources/shaders/bottle.vs", "resources/shaders/bottle.fs");
    Shader trashShader("resources/shaders/trash.vs", "resources/shaders/trash.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader plankShader("resources/shaders/plank.vs", "resources/shaders/plank.fs");


    // Skybox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // box model
    unsigned int diffuse_map = loadTexture(FileSystem::getPath("resources/textures/old-plank-flooring3_basecolor.png").c_str());
    unsigned int normal_map  = loadTexture(FileSystem::getPath("resources/textures/old-plank-flooring3_normal.png").c_str());
    unsigned int height_map  = loadTexture(FileSystem::getPath("resources/textures/old-plank-flooring3_height.png").c_str());
    unsigned int spec_map = loadTexture(FileSystem::getPath("resources/textures/old-plank-flooring3_AO.png").c_str());

    unsigned int plankVAO = 0;
    unsigned int plankVBO = 0;


    plankShader.use();
    plankShader.setInt("diffuse_map", 0);
    plankShader.setInt("normal_map", 1);
    plankShader.setInt("height_map", 2);
    plankShader.setInt("specular_map", 3);
    
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/space_ft.png"),
                    FileSystem::getPath("resources/textures/skybox/space_bk.png"),
                    FileSystem::getPath("resources/textures/skybox/space_up.png"),
                    FileSystem::getPath("resources/textures/skybox/space_dn.png"),
                    FileSystem::getPath("resources/textures/skybox/space_rt.png"),
                    FileSystem::getPath("resources/textures/skybox/space_lf.png")
            };

    unsigned int cubemapTexture = loadCubemap(faces);

    // load models
    Model dustyRoad("resources/objects/dusty_road/scene.gltf");
    dustyRoad.SetShaderTextureNamePrefix("material.");
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

    // Initializing light's components
    pointLight.position = glm::vec3(-0.59, 2.0, -1.1);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.941, 0.933, 0.612);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    dirLight.direction = glm::vec3(1.0, -2.0, 0.5);
    dirLight.ambient = glm::vec3(0.05, 0.05, -0.05);
    dirLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
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


    // render loop
    while (!glfwWindowShouldClose(window)) {

        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
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
        model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0)); 
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.0025)); 
        trashShader.setMat4("model", model);
        dustyRoad.Draw(trashShader);

        // Dumpster
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.2, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.012));    
        trashShader.setMat4("model", model);
        dumpster.Draw(trashShader);


        // Oak Tree
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0, 0.0, -3.0)); 
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.15));    
        trashShader.setMat4("model", model);
        tree.Draw(trashShader);


        // Trash Bag
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0, 0.21, 0.9)); 
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.3));    
        trashShader.setMat4("model", model);
        trashBag.Draw(trashShader);

        // Streetlight
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.2, 1.2, -1.1)); 
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.1));    
        trashShader.setMat4("model", model);
        streetLight.Draw(trashShader);


        // Pile
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.5, -0.05, -1.7)); 
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.7));    
        trashShader.setMat4("model", model);
        glDisable(GL_CULL_FACE);
        pile.Draw(trashShader);
        glEnable(GL_CULL_FACE);

        // Oil Barrel
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.3, 0.01, 1.0)); 
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        //model = glm::scale(model, glm::vec3(0.1));    
        trashShader.setMat4("model", model);
        oilBarrel.Draw(trashShader);


        // Canister
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.85, 1.75, -1.05)); 
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.009));    
        trashShader.setMat4("model", model);
        canister.Draw(trashShader);


        // Old CocaCola Can
        // view/projection transformations
        trashShader.use();
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        trashShader.setMat4("projection", projection);
        trashShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.54, 0.08, -0.5)); 
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.1));    
        trashShader.setMat4("model", model);
        trashShader.setFloat("material.shininess", 128.0);
        oldCan.Draw(trashShader);

        // Wooden plank
        plankShader.use();
        plankShader.setInt("flag", flag);
        plankShader.setVec3("dirLight.direction", dirLight.direction);
        plankShader.setVec3("dirLight.ambient", dirLight.ambient);
        plankShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        plankShader.setVec3("dirLight.specular", dirLight.specular);

        plankShader.setVec3("pointLight.position", pointLight.position);
        plankShader.setVec3("pointLight.ambient", pointLight.ambient);
        plankShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        plankShader.setVec3("pointLight.specular", pointLight.specular);
        plankShader.setFloat("pointLight.constant", pointLight.constant);
        plankShader.setFloat("pointLight.linear", pointLight.linear);
        plankShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        plankShader.setVec3("spotLight.position", programState->camera.Position);
        plankShader.setVec3("spotLight.direction", programState->camera.Front);
        plankShader.setVec3("spotLight.ambient", spotLight.ambient);
        plankShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        plankShader.setVec3("spotLight.specular", spotLight.specular);
        plankShader.setFloat("spotLight.constant", spotLight.constant);
        plankShader.setFloat("spotLight.linear", spotLight.linear);
        plankShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        plankShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        plankShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        plankShader.setVec3("viewPos", programState->camera.Position);
        plankShader.setFloat("heightScale", 0.1);
        plankShader.setFloat("shininess", 32.0);

        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        plankShader.setMat4("projection", projection);
        plankShader.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.54, 0.025, 0.2));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.5));
        plankShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse_map);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal_map);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, height_map);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, spec_map);

        renderPlank(plankVAO, plankVBO);

        // Plastic Bottle
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
        model = glm::translate(model, glm::vec3(0.25, 0.01, 0.0)); 
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.01));
        pbShader.setMat4("model", model);
        plasticBottle.Draw(pbShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.4, 0.02, -0.5)); 
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.01));
        pbShader.setMat4("model", model);
        plasticBottle.Draw(pbShader);


        // Skybox
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
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
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        static float f = 0.0f;
        ImGui::Begin("Welcome to \"Priroda i Drustvo\"!");
        ImGui::Text("Adjust lights! :)");


        ImGui::Checkbox("Moonlight", &flag1);
        if(flag1){
            flag = 1;
            flag2 = false;
            flag3 = false;
        }
        ImGui::Checkbox("Streetlight", &flag2);
        if(flag2){
            flag = 2;
            flag1 = false;
            flag3 = false;
        }
        ImGui::Checkbox("Flashlight", &flag3);
        if(flag3){
            flag = 3;
            flag1 = false;
            flag2 = false;
        }

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


unsigned int loadCubemap(vector<std::string> faces)

{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


void renderPlank(unsigned int plankVAO, unsigned int plankVBO)
{
    if (plankVAO == 0)
    {

        glm::vec3 pos1(-0.1f,  0.1f, 0.001f);
        glm::vec3 pos2(-0.1f, -1.0f, 0.001f);
        glm::vec3 pos3( 0.1f, -1.0f, 0.001f);
        glm::vec3 pos4( 0.1f,  0.1f, 0.001f);

        //        texture coordinates
        glm::vec2 uv1(0.0f, 5.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 5.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 10.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = glm::normalize(tangent1);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = glm::normalize(bitangent1);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 10.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent2 = glm::normalize(tangent2);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent2 = glm::normalize(bitangent2);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        glGenVertexArrays(1, &plankVAO);
        glGenBuffers(1, &plankVBO);
        glBindVertexArray(plankVAO);
        glBindBuffer(GL_ARRAY_BUFFER, plankVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(plankVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

