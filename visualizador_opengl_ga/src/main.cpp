// =============================================================================
// Visualizador de Cenas 3D - Pipeline Gráfico Programável
// Disciplina: Computação Gráfica
// =============================================================================
// Dependências: OpenGL 3.3+, GLFW3, GLM, Assimp, glad, ImGui
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// =============================================================================
// Constantes globais
// =============================================================================
static constexpr int   SCR_WIDTH  = 1280;
static constexpr int   SCR_HEIGHT = 720;
static constexpr float NEAR_PLANE = 0.1f;
static constexpr float FAR_PLANE  = 500.0f;

// =============================================================================
// Estado global da aplicação
// =============================================================================
Camera camera({0.0f, 1.5f, 6.0f});

float lastX      = SCR_WIDTH  / 2.0f;
float lastY      = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
bool  mouseCaptured = true; // começa capturado (modo FPS)

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Fonte de luz pontual
glm::vec3 lightPos   = {3.0f, 5.0f, 3.0f};
glm::vec3 lightColor = {1.0f, 1.0f, 1.0f};

// Modo de visualização
bool showWireframe    = false;
bool perspectiveProj  = true;  // true = perspectiva, false = ortográfica
bool showGrid         = true;
bool showAxes         = true;

// Objetos da cena
std::vector<std::unique_ptr<Model>> models;
int selectedModel = 0; // índice do objeto selecionado

// Passo de transformação por tecla
static const float TRANS_STEP   = 0.1f;
static const float ROT_STEP     = 5.0f;  // graus
static const float SCALE_STEP   = 0.05f;

// =============================================================================
// Protótipos de funções auxiliares
// =============================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void buildGridBuffers(unsigned int& vao, unsigned int& vbo, int halfSize, std::vector<float>& verts);
void buildAxesBuffers(unsigned int& vao, unsigned int& vbo);
void renderImGui();

// =============================================================================
// main
// =============================================================================
int main(int argc, char* argv[])
{
    // ---- Inicialização do GLFW ----
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                          "Visualizador 3D - Phong", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ---- Carrega ponteiros OpenGL com glad ----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar glad\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ---- Inicializa ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ---- Carrega shaders ----
    Shader phongShader   ("shaders/phong.vert",     "shaders/phong.frag");
    Shader wireShader    ("shaders/wireframe.vert",  "shaders/wireframe.frag");
    Shader gridShader    ("shaders/grid.vert",       "shaders/grid.frag");

    // ---- Carrega modelos passados como argumento ou padrão ----
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            std::cout << "Carregando: " << argv[i] << "\n";
            models.push_back(std::make_unique<Model>(argv[i]));
            // Afasta objetos automaticamente para não sobrepor
            models.back()->position.x = (float)(i - 1) * 2.5f;
        }
    } else {
        // Avisa se nenhum modelo foi fornecido
        std::cout << "Uso: visualizador <arquivo1.obj> [arquivo2.obj] ...\n";
        std::cout << "Nenhum modelo carregado. Apenas a grid e eixos serão exibidos.\n";
    }

    // ---- Grid de chão ----
    unsigned int gridVAO = 0, gridVBO = 0;
    std::vector<float> gridVerts;
    buildGridBuffers(gridVAO, gridVBO, 10, gridVerts);

    // ---- Eixos (X vermelho, Y verde, Z azul) ----
    unsigned int axesVAO = 0, axesVBO = 0;
    buildAxesBuffers(axesVAO, axesVBO);

    // ==========================================================================
    // Loop principal de renderização
    // ==========================================================================
    while (!glfwWindowShouldClose(window)) {
        // Calcula deltaTime para movimento independente de FPS
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Limpa os buffers de cor e profundidade
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Obtém dimensões atuais da janela (pode ter sido redimensionada)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 1.0f;

        // ---- Matrizes de câmera ----
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection;
        if (perspectiveProj) {
            projection = glm::perspective(glm::radians(camera.Fov), aspect, NEAR_PLANE, FAR_PLANE);
        } else {
            float orthoHeight = 5.0f;
            float orthoWidth  = orthoHeight * aspect;
            projection = glm::ortho(-orthoWidth, orthoWidth,
                                    -orthoHeight, orthoHeight,
                                    NEAR_PLANE, FAR_PLANE);
        }

        // ==================================================================
        // 1. Renderiza a Grid de chão
        // ==================================================================
        if (showGrid) {
            gridShader.use();
            gridShader.setMat4("view",       view);
            gridShader.setMat4("projection", projection);
            gridShader.setVec3("gridColor",  {0.4f, 0.4f, 0.4f});

            glBindVertexArray(gridVAO);
            glDrawArrays(GL_LINES, 0, (GLsizei)(gridVerts.size() / 3));
            glBindVertexArray(0);
        }

        // ==================================================================
        // 2. Renderiza os Eixos (X Y Z)
        // ==================================================================
        if (showAxes) {
            wireShader.use();
            wireShader.setMat4("model",      glm::mat4(1.0f));
            wireShader.setMat4("view",       view);
            wireShader.setMat4("projection", projection);

            glBindVertexArray(axesVAO);
            // Eixo X → vermelho
            wireShader.setVec3("wireColor", {1.0f, 0.0f, 0.0f});
            glDrawArrays(GL_LINES, 0, 2);
            // Eixo Y → verde
            wireShader.setVec3("wireColor", {0.0f, 1.0f, 0.0f});
            glDrawArrays(GL_LINES, 2, 2);
            // Eixo Z → azul
            wireShader.setVec3("wireColor", {0.0f, 0.0f, 1.0f});
            glDrawArrays(GL_LINES, 4, 2);
            glBindVertexArray(0);
        }

        // ==================================================================
        // 3. Renderiza os modelos com iluminação de Phong
        // ==================================================================
        phongShader.use();
        phongShader.setMat4("view",       view);
        phongShader.setMat4("projection", projection);
        phongShader.setVec3("lightPos",   lightPos);
        phongShader.setVec3("lightColor", lightColor);
        phongShader.setVec3("viewPos",    camera.Position);

        for (int i = 0; i < (int)models.size(); i++) {
            // Ajusta ka levemente para destacar o objeto selecionado
            PhongMaterial mat = models[i]->material;
            if (i == selectedModel)
                mat.ka = mat.ka + glm::vec3(0.08f); // pequeno brilho extra

            models[i]->material = mat; // (temporário, restaurado abaixo)
            models[i]->draw(phongShader);
        }

        // ==================================================================
        // 4. Sobrepõe wireframe (opcional)
        // ==================================================================
        if (showWireframe && !models.empty()) {
            glPolygonOffset(1.0f, 1.0f);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            wireShader.use();
            wireShader.setMat4("view",       view);
            wireShader.setMat4("projection", projection);
            wireShader.setVec3("wireColor",  {0.0f, 0.8f, 0.8f});

            for (auto& model : models) {
                wireShader.setMat4("model", model->getModelMatrix());
                for (auto& mesh : model->meshes)
                    mesh.draw();
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }

        // ==================================================================
        // 5. Interface ImGui
        // ==================================================================
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderImGui();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    for (auto& m : models) m->free();
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// =============================================================================
// Callbacks e processamento de entrada
// =============================================================================

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    if (!mouseCaptured) return;

    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse) {
        lastX      = xpos;
        lastY      = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // invertido: y cresce para cima em OpenGL
    lastX = xpos;
    lastY = ypos;

    camera.processMouse(xoffset, yoffset);
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    camera.processScroll((float)yoffset);
}

// Teclas de controle de câmera e transformações do objeto selecionado
void processInput(GLFWwindow* window)
{
    // Fecha a janela
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Alterna captura do mouse (Tab)
    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        tabPressed = true;
        mouseCaptured = !mouseCaptured;
        firstMouse    = true;
        glfwSetInputMode(window, GLFW_CURSOR,
                         mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) tabPressed = false;

    // ---- Câmera (WASD + Q/E) ----
    if (mouseCaptured) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(FORWARD,  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(LEFT,     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(RIGHT,    deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.processKeyboard(DOWN,     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.processKeyboard(UP,       deltaTime);
    }

    if (models.empty()) return;
    Model& sel = *models[selectedModel];

    // ---- Seleção de objeto (1–9) ----
    for (int i = 0; i < (int)models.size() && i < 9; i++) {
        if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS)
            selectedModel = i;
    }

    // ---- Translação (I/K = Y, J/L = X, U/O = Z) ----
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) sel.position.y += TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) sel.position.y -= TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) sel.position.x -= TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) sel.position.x += TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) sel.position.z += TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) sel.position.z -= TRANS_STEP;

    // ---- Rotação (F1=X, F2=Y, F3=Z; Shift inverte) ----
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    float dir  = shift ? -1.0f : 1.0f;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) sel.rotation.x += dir * ROT_STEP;
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) sel.rotation.y += dir * ROT_STEP;
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) sel.rotation.z += dir * ROT_STEP;

    // ---- Escala uniforme (+ / -) ----
    static bool plusP = false, minusP = false;
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS && !plusP) {
        plusP = true;
        sel.scale += glm::vec3(SCALE_STEP);
    }
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_RELEASE) plusP = false;
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS && !minusP) {
        minusP = true;
        sel.scale = glm::max(sel.scale - glm::vec3(SCALE_STEP), glm::vec3(0.01f));
    }
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_RELEASE) minusP = false;

    // ---- Wireframe (V) ----
    static bool vPressed = false;
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vPressed) {
        vPressed = true;
        showWireframe = !showWireframe;
    }
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) vPressed = false;

    // ---- Projeção (P) ----
    static bool pPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) {
        pPressed = true;
        perspectiveProj = !perspectiveProj;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) pPressed = false;
}

// =============================================================================
// Constrói VAO/VBO de uma grid de linhas no plano XZ
// =============================================================================
void buildGridBuffers(unsigned int& vao, unsigned int& vbo,
                      int halfSize, std::vector<float>& verts)
{
    for (int i = -halfSize; i <= halfSize; i++) {
        // Linhas paralelas ao Z
        verts.insert(verts.end(), { (float)i, 0.0f, (float)-halfSize });
        verts.insert(verts.end(), { (float)i, 0.0f, (float) halfSize });
        // Linhas paralelas ao X
        verts.insert(verts.end(), { (float)-halfSize, 0.0f, (float)i });
        verts.insert(verts.end(), { (float) halfSize, 0.0f, (float)i });
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

// =============================================================================
// Constrói VAO/VBO dos eixos (X, Y, Z)
// =============================================================================
void buildAxesBuffers(unsigned int& vao, unsigned int& vbo)
{
    float axisVerts[] = {
        0,0,0,  2,0,0,   // X
        0,0,0,  0,2,0,   // Y
        0,0,0,  0,0,2    // Z
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVerts), axisVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

// =============================================================================
// Painel ImGui com controles completos
// =============================================================================
void renderImGui()
{
    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({340, 0}, ImGuiCond_Always);
    ImGui::Begin("Visualizador 3D", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    // ---- Ajuda ----
    if (ImGui::CollapsingHeader("Controles (teclado)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Câmera: WASD / Q(baixo) / E(cima)");
        ImGui::TextDisabled("Mouse: rotaciona câmera  |  Tab: libera mouse");
        ImGui::TextDisabled("Teclas 1-9: seleciona objeto");
        ImGui::TextDisabled("IJKL + U/O: translação XYZ");
        ImGui::TextDisabled("F1/F2/F3 (+Shift): rotação XYZ");
        ImGui::TextDisabled("NumPad+/-: escala uniforme");
        ImGui::TextDisabled("V: wireframe   P: projeção   Esc: sair");
    }

    // ---- Cena ----
    if (ImGui::CollapsingHeader("Cena", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Grid",       &showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Eixos",      &showAxes);
        ImGui::SameLine();
        ImGui::Checkbox("Wireframe",  &showWireframe);

        const char* projLabel = perspectiveProj ? "Perspectiva" : "Ortografica";
        if (ImGui::Button(projLabel)) perspectiveProj = !perspectiveProj;
    }

    // ---- Luz ----
    if (ImGui::CollapsingHeader("Luz Pontual")) {
        ImGui::DragFloat3("Posição",      &lightPos.x,   0.1f);
        ImGui::ColorEdit3("Cor da Luz",   &lightColor.x);
    }

    // ---- Objeto selecionado ----
    if (!models.empty()) {
        if (ImGui::CollapsingHeader("Objeto Selecionado", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Combo de seleção
            std::vector<const char*> names;
            for (auto& m : models) names.push_back(m->name.c_str());
            ImGui::Combo("Objeto", &selectedModel, names.data(), (int)names.size());

            Model& sel = *models[selectedModel];

            ImGui::Separator();
            ImGui::DragFloat3("Posição##obj",  &sel.position.x, 0.01f);
            ImGui::DragFloat3("Rotação##obj",  &sel.rotation.x, 0.5f);
            ImGui::DragFloat3("Escala##obj",   &sel.scale.x,    0.01f, 0.01f, 50.0f);

            ImGui::Separator();
            ImGui::Text("Material (Phong)");
            ImGui::ColorEdit3("Ka (ambiente)",  &sel.material.ka.x);
            ImGui::ColorEdit3("Kd (difuso)",    &sel.material.kd.x);
            ImGui::ColorEdit3("Ks (especular)", &sel.material.ks.x);
            ImGui::SliderFloat("Brilho (n)",    &sel.material.shininess, 1.0f, 256.0f);
        }
    } else {
        ImGui::TextColored({1,0.4f,0.4f,1}, "Nenhum modelo carregado.");
        ImGui::TextWrapped("Passe arquivos .obj/.ply como argumento:\nvisualizar modelo.obj");
    }

    ImGui::End();
}
