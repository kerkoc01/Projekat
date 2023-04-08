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

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight{
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
};

struct DirLight{
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 Position = glm::vec3(0.0f);
    float Scale = 1.0f;
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

void DrawImGui(ProgramState *programState);

unsigned int loadTexture(char const * path);

void bindPointLight(Shader &shader, PointLight pointLight);

void bindCameraPosition(Shader &shader, glm::vec3 position);

void bindShininess(Shader &shader, float value);

void setShaderViewMatrix(Shader &shader, glm::mat4 view);

void setShaderProjectionMatrix(Shader &shader, glm::mat4 projection);

void setShaderModelMatrix(Shader &shader, glm::mat4 model);

void enableShaderDiffuseComponent(Shader &shader);

void enableShaderSpecularComponent(Shader &shader);

void bindSpotLight(Shader &shader, SpotLight spotLight);

void bindDirLight(Shader &shader, DirLight dirLight);

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
    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // build and compile shaders
    // -------------------------
    Shader mainShader("resources/shaders/mainShader.vs", "resources/shaders/mainShader.fs");
    Shader grassShader("resources/shaders/grassShader.vs", "resources/shaders/grassShader.fs");
    Shader planeShader("resources/shaders/planeShader.vs", "resources/shaders/planeShader.fs");

    // load models
    // -----------
    Model goalModel("resources/objects/goalpost/10502_Football_Goalpost_v1_L3.obj");
    goalModel.SetShaderTextureNamePrefix("material.");

    Model projectorModel("resources/objects/projector/projector_mast.obj");
    projectorModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(1.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(2.0, 2.0, 2.0);
    pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight.specular = glm::vec3(0.1, 0.1, 0.1);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    DirLight dirLight;
    dirLight.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    dirLight.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    dirLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);

    SpotLight spotLight;
    spotLight.position = glm::vec3(20.0f, 50.0f, 20.0f);
    spotLight.direction = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
    spotLight.cutOff = glm::cos(glm::radians(12.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(17.5f));
    spotLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    spotLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    spotLight.specular = glm::vec3(1.0, 1.0, 1.0);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;


    //initializing vertices

    float planeVertices[] = {
            //position                       normals                       texture
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,-1.0f, 1.0f,
            -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,-1.0f, -1.0f,

            1.0f, 0.0f, 1.0f,0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,-1.0f, -1.0f,
            1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,1.0f, -1.0f
    };

    float grassVertices[] = {

            //position                          normals                 texture
            0.0f,  0.5f,  0.0f, sqrt(2.0f), 0, sqrt(2.0f), 0.0f,  0.0f,
            0.0f, -0.5f,  0.0f, sqrt(2.0f), 0, sqrt(2.0f), 0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  sqrt(2.0f), 0, sqrt(2.0f),1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  sqrt(2.0f), 0, sqrt(2.0f),0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  sqrt(2.0f), 0, sqrt(2.0f),1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  sqrt(2.0f), 0, sqrt(2.0f),1.0f,  0.0f

    };

    //making buffers

    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);


    unsigned int grassVAO, grassVBO;
    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &grassVBO);
    glBindVertexArray(grassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grassVertices), &grassVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);


    //loading textures

    unsigned int grassTextureDiffuse = loadTexture(FileSystem::getPath("resources/textures/grass_texture.png").c_str()); // Downloaded texture from https://github.com/Vulpinii/grass-tutorial_codebase/blob/master/assets/textures/grass_texture.png
    unsigned int grassTextureSpecular = loadTexture(FileSystem::getPath("resources/textures/grass_texture_specular.psd").c_str());
    unsigned int planeTexture = loadTexture(FileSystem::getPath("resources/textures/plane_texture.jpg").c_str());


    grassShader.use();
    enableShaderDiffuseComponent(grassShader);
    enableShaderSpecularComponent(grassShader);

    planeShader.use();
    planeShader.setInt("texture1", 0);

    //calculating grass position

    vector<glm::vec3> grassPosition;
    for(int i = 0;i < 50;i++)
        for(int j = 0;j < 50;j++)
            grassPosition.push_back(glm::vec3(i - 25.0f, 0.3f, j - 25.0f));

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


        mainShader.use();
        pointLight.position = glm::vec3(10.0, 10.0, 10.0);
        bindPointLight(mainShader, pointLight);
        bindCameraPosition(mainShader, programState->camera.Position);
        bindShininess(mainShader, 32.0f);


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        setShaderProjectionMatrix(mainShader, projection);
        setShaderViewMatrix(mainShader, view);

        // render loaded models

        //goal
        glCullFace(GL_BACK);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0f));
        model = glm::scale(model, glm::vec3(0.01f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        setShaderModelMatrix(mainShader, model);
        goalModel.Draw(mainShader);

        //projector
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(20.0f, 0.0f, 20.0f));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(1.5f));
        setShaderModelMatrix(mainShader, model);
        projectorModel.Draw(mainShader);

        //plane
        planeShader.use();
        glCullFace(GL_FRONT);
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, planeTexture);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        setShaderProjectionMatrix(planeShader, projection);
        setShaderViewMatrix(planeShader, view);
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(21.0f));
        setShaderModelMatrix(planeShader, model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //grass
        glDisable(GL_CULL_FACE);
        grassShader.use();
        bindShininess(grassShader, 16.0f);
        bindSpotLight(grassShader, spotLight);
        bindDirLight(grassShader, dirLight);
        glBindVertexArray(grassVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTextureDiffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grassTextureSpecular);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        setShaderProjectionMatrix(grassShader, projection);
        setShaderViewMatrix(grassShader, view);
        for(auto i : grassPosition){
            model = glm::mat4(1.0f);
            model = glm::translate(model, i);
            for(int j = 0;j < 3;j++) {
                model = glm::rotate(model, glm::radians(120.0f), glm::vec3(0, 1, 0));
                model = glm::scale(model, glm::vec3(1.6f, 1.0f, 1.6f));
                setShaderModelMatrix(grassShader, model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        glEnable(GL_CULL_FACE);

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
    glDeleteVertexArrays(1, &grassVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &grassVAO);
    glDeleteBuffers(1, &planeVAO);
    glfwTerminate();
    return 0;
}

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

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

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
        ImGui::DragFloat3("Backpack position", (float*)&programState->Position);
        ImGui::DragFloat("Backpack scale", &programState->Scale, 0.05, 0.1, 4.0);

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

unsigned int loadTexture(char const * path)
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
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

void bindPointLight(Shader &shader, PointLight pointLight){
    shader.setVec3("pointLight.position", pointLight.position);
    shader.setVec3("pointLight.ambient", pointLight.ambient);
    shader.setVec3("pointLight.diffuse", pointLight.diffuse);
    shader.setVec3("pointLight.specular", pointLight.specular);
    shader.setFloat("pointLight.constant", pointLight.constant);
    shader.setFloat("pointLight.linear", pointLight.linear);
    shader.setFloat("pointLight.quadratic", pointLight.quadratic);
}

void bindCameraPosition(Shader &shader, glm::vec3 position){
    shader.setVec3("viewPosition", position);
}

void bindShininess(Shader &shader, float value){
    shader.setFloat("material.shininess", value);
}

void setShaderViewMatrix(Shader &shader, glm::mat4 view){
    shader.setMat4("view", view);
}

void setShaderProjectionMatrix(Shader &shader, glm::mat4 projection){
    shader.setMat4("projection", projection);
}

void setShaderModelMatrix(Shader &shader, glm::mat4 model){
    shader.setMat4("model", model);
}

void enableShaderDiffuseComponent(Shader &shader){
    shader.setInt("material.diffuse", 0);
}

void enableShaderSpecularComponent(Shader &shader){
    shader.setInt("material.specular", 1);
}

void bindSpotLight(Shader &shader, SpotLight spotLight){
    shader.setVec3("shader.position", spotLight.position);
    shader.setVec3("shader.direction", spotLight.direction);
    shader.setFloat("shader.cutOff", spotLight.cutOff);
    shader.setFloat("shader.outerCutOff", spotLight.outerCutOff);
    shader.setVec3("shader.ambient", spotLight.ambient);
    shader.setVec3("shader.diffuse", spotLight.diffuse);
    shader.setVec3("shader.specular", spotLight.specular);
    shader.setFloat("shader.constant", spotLight.constant);
    shader.setFloat("shader.linear", spotLight.linear);
    shader.setFloat("shader.quadratic", spotLight.quadratic);
}

void bindDirLight(Shader &shader, DirLight dirLight){
    shader.setVec3("shader.direction", dirLight.direction);
    shader.setVec3("shader.ambient", dirLight.ambient);
    shader.setVec3("shader.diffuse", dirLight.diffuse);
    shader.setVec3("shader.specular", dirLight.specular);
}
