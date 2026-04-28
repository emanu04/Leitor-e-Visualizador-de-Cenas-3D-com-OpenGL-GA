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

// Ponto de entrada do visualizador 3D com iluminação Phong.
// Responsabilidades do main():
//   - GLFW: cria a janela e o contexto OpenGL 3.3 Core Profile
//   - GLAD: carrega os ponteiros das funções OpenGL em tempo de execução
//   - ImGui: configura a interface gráfica de parâmetros (painel lateral)
//   - Shaders: compila 3 pares — phong (modelos), wireframe (arestas/eixos) e grid (grade do chão)
//   - Loop de renderização: atualiza entrada, limpa buffers, desenha grid/eixos/modelos/UI

// Dimensões da janela e planos de clipping da câmera
static constexpr int   SCR_WIDTH  = 1280;
static constexpr int   SCR_HEIGHT = 720;
static constexpr float NEAR_PLANE = 0.1f;   // objetos mais próximos que isso são cortados
static constexpr float FAR_PLANE  = 500.0f; // objetos mais distantes que isso são cortados

// Câmera FPS posicionada ligeiramente acima e à frente da origem
Camera camera({0.0f, 1.5f, 6.0f});

// Última posição do mouse — usada para calcular o deslocamento (delta) entre frames
float lastX      = SCR_WIDTH  / 2.0f;
float lastY      = SCR_HEIGHT / 2.0f;
// firstMouse evita um salto brusco de câmera quando o mouse é capturado pela primeira vez
bool  firstMouse = true;
bool  mouseCaptured = true; // true = mouse capturado (modo FPS); false = cursor livre para ImGui

// Tempo entre frames em segundos — usado para tornar o movimento independente do frame rate
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Fonte de luz pontual: posição no espaço mundo e cor (RGB normalizado)
glm::vec3 lightPos   = {3.0f, 5.0f, 3.0f};
glm::vec3 lightColor = {1.0f, 1.0f, 1.0f};

// Flags de modo de visualização — alternáveis por tecla ou pela interface ImGui
bool showWireframe    = false; // sobrepõe arestas sobre os modelos sólidos
bool perspectiveProj  = true;  // true = perspectiva (FOV), false = ortográfica (sem profundidade)
bool showGrid         = true;  // exibe a grade do plano Y=0
bool showAxes         = true;  // exibe os eixos XYZ coloridos na origem

// Lista de modelos carregados na cena e índice do modelo atualmente selecionado
std::vector<std::unique_ptr<Model>> models;
int selectedModel = 0; // índice em models[]; o objeto selecionado recebe destaque visual

// Incrementos por pressionamento de tecla para as transformações do objeto selecionado
static const float TRANS_STEP   = 0.1f;  // unidades de mundo por quadro
static const float ROT_STEP     = 5.0f;  // graus por quadro
static const float SCALE_STEP   = 0.05f; // fator de escala por pressionamento

// Declarações antecipadas das funções de callback e auxiliares
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void buildGridBuffers(unsigned int& vao, unsigned int& vbo, int halfSize, std::vector<float>& verts);
void buildAxesBuffers(unsigned int& vao, unsigned int& vbo);
void renderImGui();


int main(int argc, char* argv[])
{
    // --- Inicialização do GLFW ---
    // GLFW gerencia a janela e o contexto OpenGL de forma portável entre sistemas operacionais
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW\n";
        return -1;
    }
    // Requisita OpenGL 3.3 Core Profile (sem funções legadas deprecated)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS exige FORWARD_COMPAT para OpenGL 3.3+ funcionar corretamente
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

    // Registra callbacks: redimensionamento de janela, movimento do mouse e scroll
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // Oculta e captura o cursor para modo FPS (posição relativa ao centro)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // --- Inicialização do GLAD ---
    // GLAD carrega os ponteiros das funções OpenGL em tempo de execução.
    // Sem isso, chamadas como glDrawElements seriam ponteiros nulos.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar glad\n";
        return -1;
    }

    // Ativa o teste de profundidade: fragmentos atrás de outros são descartados
    glEnable(GL_DEPTH_TEST);
    // Ativa blending para transparência (alfa) — usado pelo ImGui
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Inicialização do ImGui ---
    // ImGui é uma biblioteca de interface gráfica imediata (immediate mode GUI):
    // cada frame a UI é recriada do zero com base no estado atual das variáveis.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330"); // versão GLSL compatível com o contexto 3.3

    // --- Carregamento dos Shaders ---
    // phongShader: iluminação Phong completa para os modelos 3D
    // wireShader:  cor sólida sem iluminação — usado para eixos e overlay de wireframe
    // gridShader:  grade do plano Y=0 com cor uniforme
    Shader phongShader   ("shaders/phong.vert",     "shaders/phong.frag");
    Shader wireShader    ("shaders/wireframe.vert",  "shaders/wireframe.frag");
    Shader gridShader    ("shaders/grid.vert",       "shaders/grid.frag");

    // --- Carregamento dos Modelos ---
    // Cada argumento da linha de comando é um arquivo 3D (.obj, .ply, etc.)
    // Modelos são espaçados em 2.5 unidades no eixo X para não se sobrepor
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            std::cout << "Carregando: " << argv[i] << "\n";
            models.push_back(std::make_unique<Model>(argv[i]));
            models.back()->position.x = (float)(i - 1) * 2.5f;

            // Material inicial genérico — pode ser ajustado em tempo real pelo ImGui
            models.back()->material.ka = glm::vec3(0.1f, 0.1f, 0.1f); // luz ambiente baixa
            models.back()->material.kd = glm::vec3(0.6f, 0.6f, 0.6f); // cor base cinza
            models.back()->material.ks = glm::vec3(0.5f, 0.5f, 0.5f); // brilho moderado
            models.back()->material.shininess = 32.0f;                 // highlight médio
        }
    } else {
        std::cout << "Uso: visualizador <arquivo1.obj> [arquivo2.obj] ...\n";
        std::cout << "Nenhum modelo carregado. Apenas a grid e eixos serão exibidos.\n";
    }

    // --- Construção dos Buffers de Grade e Eixos ---
    // A grade é um conjunto de linhas no plano Y=0 geradas pela CPU e enviadas à GPU
    unsigned int gridVAO = 0, gridVBO = 0;
    std::vector<float> gridVerts;
    buildGridBuffers(gridVAO, gridVBO, 10, gridVerts); // grade de 20x20 unidades (halfSize=10)

    // Os eixos são 3 segmentos de reta coloridos (vermelho=X, verde=Y, azul=Z)
    unsigned int axesVAO = 0, axesVBO = 0;
    buildAxesBuffers(axesVAO, axesVBO);

    // =========================================================================
    // Loop de renderização principal — executa até a janela ser fechada
    // =========================================================================
    while (!glfwWindowShouldClose(window)) {
        // Calcula o tempo decorrido desde o último frame para normalizar movimento
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Processa teclas pressionadas (câmera e transformações de objetos)
        processInput(window);

        // Limpa o framebuffer com cor de fundo escura e reinicia o buffer de profundidade
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Obtém as dimensões reais do framebuffer (pode diferir de SCR_WIDTH/HEIGHT em telas HiDPI)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 1.0f;

        // Monta a view matrix (posição e orientação da câmera)
        glm::mat4 view = camera.getViewMatrix();

        // Monta a projection matrix com base no modo selecionado:
        //   Perspectiva: objetos distantes parecem menores (visão natural)
        //   Ortográfica: sem distorção de perspectiva (útil para modelagem técnica)
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

        // --- Passo 1: Renderiza a grade do chão ---
        // A grade não tem model matrix (fica no espaço mundo diretamente)
        if (showGrid) {
            gridShader.use();
            gridShader.setMat4("view",       view);
            gridShader.setMat4("projection", projection);
            gridShader.setVec3("gridColor",  {0.4f, 0.4f, 0.4f}); // cinza médio

            glBindVertexArray(gridVAO);
            glDrawArrays(GL_LINES, 0, (GLsizei)(gridVerts.size() / 3));
            glBindVertexArray(0);
        }

        // --- Passo 2: Renderiza os eixos XYZ ---
        // Cada eixo é um par de vértices desenhado com cor diferente via uniform wireColor
        if (showAxes) {
            wireShader.use();
            wireShader.setMat4("model",      glm::mat4(1.0f)); // identidade = na origem
            wireShader.setMat4("view",       view);
            wireShader.setMat4("projection", projection);

            glBindVertexArray(axesVAO);
            wireShader.setVec3("wireColor", {1.0f, 0.0f, 0.0f}); // X = vermelho
            glDrawArrays(GL_LINES, 0, 2);
            wireShader.setVec3("wireColor", {0.0f, 1.0f, 0.0f}); // Y = verde
            glDrawArrays(GL_LINES, 2, 2);
            wireShader.setVec3("wireColor", {0.0f, 0.0f, 1.0f}); // Z = azul
            glDrawArrays(GL_LINES, 4, 2);
            glBindVertexArray(0);
        }

        // --- Passo 3: Renderiza os modelos com iluminação Phong ---
        phongShader.use();
        phongShader.setMat4("view",       view);
        phongShader.setMat4("projection", projection);
        phongShader.setVec3("lightPos",   lightPos);
        phongShader.setVec3("lightColor", lightColor);
        phongShader.setVec3("viewPos",    camera.Position); // necessário para o cálculo especular

        for (int i = 0; i < (int)models.size(); i++) {
            PhongMaterial materialOriginal = models[i]->material;

            // O modelo selecionado recebe um leve aumento no componente ambiente (ka)
            // para destacá-lo visualmente sem alterar os parâmetros de material permanentemente
            PhongMaterial mat = materialOriginal;
            if (i == selectedModel)
                mat.ka = mat.ka + glm::vec3(0.08f);

            models[i]->material = mat;
            models[i]->draw(phongShader);

            // Restaura o material original para não afetar o painel ImGui
            models[i]->material = materialOriginal;
        }

        // --- Passo 4: Overlay de wireframe (opcional) ---
        // Desenhamos os mesmos modelos novamente em modo GL_LINE sobre os sólidos.
        // glPolygonOffset afasta ligeiramente as linhas do sólido para evitar z-fighting
        // (artefato onde duas superfícies coplanares brigam pelo mesmo pixel de profundidade).
        if (showWireframe && !models.empty()) {
            glPolygonOffset(1.0f, 1.0f);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // muda modo de rasterização para linhas

            wireShader.use();
            wireShader.setMat4("view",       view);
            wireShader.setMat4("projection", projection);
            wireShader.setVec3("wireColor",  {0.0f, 0.8f, 0.8f}); // ciano

            for (auto& model : models) {
                wireShader.setMat4("model", model->getModelMatrix());
                for (auto& mesh : model->meshes)
                    mesh.draw();
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // restaura modo sólido
            glDisable(GL_POLYGON_OFFSET_LINE);
        }

        // --- Passo 5: Renderiza a interface ImGui ---
        // ImGui é desenhada por último (sem depth test efetivo) para ficar sempre visível
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderImGui(); // monta os widgets do painel
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Troca os buffers (double buffering) e processa eventos pendentes do sistema
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Limpeza de recursos ---
    // Libera buffers de GPU dos modelos
    for (auto& m : models) m->free();
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);

    // Encerra ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Encerra GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


// Callback chamado pelo GLFW quando a janela é redimensionada.
// Atualiza o viewport para usar toda a área do framebuffer.
void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Callback chamado pelo GLFW a cada movimento do mouse.
// Converte a posição absoluta em delta (diferença em relação ao frame anterior)
// e repassa para a câmera. O eixo Y é invertido porque GLFW cresce para baixo,
// mas queremos que mover o mouse para cima incline a câmera para cima (pitch positivo).
void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    if (!mouseCaptured) return; // ignora quando o cursor está livre (modo ImGui)

    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    // No primeiro frame com o mouse capturado, inicializa lastX/Y para evitar salto brusco
    if (firstMouse) {
        lastX      = xpos;
        lastY      = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // invertido: mouse para cima → yoffset positivo → Pitch sobe
    lastX = xpos;
    lastY = ypos;

    camera.processMouse(xoffset, yoffset);
}

// Callback chamado pelo GLFW quando o usuário usa o scroll do mouse.
// Repassa o delta vertical para ajustar o campo de visão (zoom) da câmera.
void scroll_callback(GLFWwindow*, double, double yoffset)
{
    camera.processScroll((float)yoffset);
}

// Processa todas as entradas de teclado a cada frame.
// Inclui: fechar janela, alternar captura do mouse, mover câmera,
// selecionar objeto (1-9), transladar/rotacionar/escalar o objeto selecionado,
// e alternar wireframe e tipo de projeção.
void processInput(GLFWwindow* window)
{
    // ESC fecha a janela
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Tab alterna entre modo FPS (mouse capturado) e modo UI (cursor livre para ImGui).
    // firstMouse é resetado para evitar salto ao reativar o modo FPS.
    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        tabPressed = true;
        mouseCaptured = !mouseCaptured;
        firstMouse    = true;
        glfwSetInputMode(window, GLFW_CURSOR,
                         mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) tabPressed = false;

    // WASD + Q/E: movimento da câmera (apenas quando o mouse está capturado)
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

    // Teclas 1-9 selecionam o modelo correspondente (índice base 0)
    for (int i = 0; i < (int)models.size() && i < 9; i++) {
        if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS)
            selectedModel = i;
    }

    // IJKL + U/O: translação do objeto selecionado nos eixos X, Y e Z
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) sel.position.y += TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) sel.position.y -= TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) sel.position.x -= TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) sel.position.x += TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) sel.position.z += TRANS_STEP;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) sel.position.z -= TRANS_STEP;

    // F1/F2/F3: rotação nos eixos X/Y/Z. Shift invertido inverte o sentido.
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    float dir  = shift ? -1.0f : 1.0f;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) sel.rotation.x += dir * ROT_STEP;
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) sel.rotation.y += dir * ROT_STEP;
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) sel.rotation.z += dir * ROT_STEP;

    // NumPad +/-: escala uniforme. Usa flag estática para disparar uma vez por pressionamento
    // (evita escala contínua enquanto a tecla fica pressionada).
    static bool plusP = false, minusP = false;
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS && !plusP) {
        plusP = true;
        sel.scale += glm::vec3(SCALE_STEP);
    }
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_RELEASE) plusP = false;
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS && !minusP) {
        minusP = true;
        // glm::max garante que a escala nunca chegue a zero (o que causaria inversão do objeto)
        sel.scale = glm::max(sel.scale - glm::vec3(SCALE_STEP), glm::vec3(0.01f));
    }
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_RELEASE) minusP = false;

    // V: alterna o modo wireframe
    static bool vPressed = false;
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vPressed) {
        vPressed = true;
        showWireframe = !showWireframe;
    }
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) vPressed = false;

    // P: alterna entre projeção perspectiva e ortográfica
    static bool pPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) {
        pPressed = true;
        perspectiveProj = !perspectiveProj;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) pPressed = false;
}


// Gera os vértices e buffers da grade do chão no plano Y=0.
// A grade tem (2*halfSize+1) linhas em cada direção, formando quadrados de 1 unidade.
// halfSize=10 gera uma grade de 20x20 unidades centrada na origem.
void buildGridBuffers(unsigned int& vao, unsigned int& vbo,
                      int halfSize, std::vector<float>& verts)
{
    for (int i = -halfSize; i <= halfSize; i++) {
        // Linhas verticais (paralelas ao eixo Z)
        verts.insert(verts.end(), { (float)i, 0.0f, (float)-halfSize });
        verts.insert(verts.end(), { (float)i, 0.0f, (float) halfSize });
        // Linhas horizontais (paralelas ao eixo X)
        verts.insert(verts.end(), { (float)-halfSize, 0.0f, (float)i });
        verts.insert(verts.end(), { (float) halfSize, 0.0f, (float)i });
    }

    // Cria VAO e VBO, envia os vértices e configura o atributo de posição (location 0)
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}


// Gera os buffers dos três eixos de coordenadas (XYZ) como segmentos de reta.
// Cada eixo é armazenado como um par de vértices: origem + ponta (comprimento 2 unidades).
// A cor de cada eixo é definida no loop de renderização via uniform wireColor.
void buildAxesBuffers(unsigned int& vao, unsigned int& vbo)
{
    float axisVerts[] = {
        0,0,0,  2,0,0,   // eixo X: da origem até (2,0,0) → vermelho
        0,0,0,  0,2,0,   // eixo Y: da origem até (0,2,0) → verde
        0,0,0,  0,0,2    // eixo Z: da origem até (0,0,2) → azul
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


// Monta e exibe o painel lateral do ImGui.
// O painel está fixo no canto superior esquerdo e exibe:
//   - Referência rápida dos controles de teclado
//   - Toggles de cena (grid, eixos, wireframe, projeção)
//   - Controles da luz pontual (posição e cor)
//   - Transformações e material Phong do objeto selecionado
void renderImGui()
{
    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({340, 0}, ImGuiCond_Always); // largura fixa, altura automática
    ImGui::Begin("Visualizador 3D", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    // Seção de ajuda com os atalhos de teclado
    if (ImGui::CollapsingHeader("Controles (teclado)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Câmera: WASD / Q(baixo) / E(cima)");
        ImGui::TextDisabled("Mouse: rotaciona câmera  |  Tab: libera mouse");
        ImGui::TextDisabled("Teclas 1-9: seleciona objeto");
        ImGui::TextDisabled("IJKL + U/O: translação XYZ");
        ImGui::TextDisabled("F1/F2/F3 (+Shift): rotação XYZ");
        ImGui::TextDisabled("NumPad+/-: escala uniforme");
        ImGui::TextDisabled("V: wireframe   P: projeção   Esc: sair");
    }

    // Seção de configurações globais da cena
    if (ImGui::CollapsingHeader("Cena", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Grid",       &showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Eixos",      &showAxes);
        ImGui::SameLine();
        ImGui::Checkbox("Wireframe",  &showWireframe);

        // Botão que mostra o modo atual e alterna ao clicar
        const char* projLabel = perspectiveProj ? "Perspectiva" : "Ortografica";
        if (ImGui::Button(projLabel)) perspectiveProj = !perspectiveProj;
    }

    // Seção de controle da fonte de luz pontual
    if (ImGui::CollapsingHeader("Luz Pontual")) {
        ImGui::DragFloat3("Posição",      &lightPos.x,   0.1f);
        ImGui::ColorEdit3("Cor da Luz",   &lightColor.x);
    }

    // Seção de controle do objeto selecionado (só aparece se houver modelos)
    if (!models.empty()) {
        if (ImGui::CollapsingHeader("Objeto Selecionado", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Dropdown para escolher qual modelo editar
            std::vector<const char*> names;
            for (auto& m : models) names.push_back(m->name.c_str());
            ImGui::Combo("Objeto", &selectedModel, names.data(), (int)names.size());

            Model& sel = *models[selectedModel];

            ImGui::Separator();
            // Controles de transformação: DragFloat permite arrastar ou digitar valores
            ImGui::DragFloat3("Posição##obj",  &sel.position.x, 0.01f);
            ImGui::DragFloat3("Rotação##obj",  &sel.rotation.x, 0.5f);
            ImGui::DragFloat3("Escala##obj",   &sel.scale.x,    0.01f, 0.01f, 50.0f);

            ImGui::Separator();
            ImGui::Text("Material (Phong)");
            // ColorEdit3 abre um seletor de cor para cada coeficiente Phong
            ImGui::ColorEdit3("Ka (ambiente)",  &sel.material.ka.x);
            ImGui::ColorEdit3("Kd (difuso)",    &sel.material.kd.x);
            ImGui::ColorEdit3("Ks (especular)", &sel.material.ks.x);
            ImGui::SliderFloat("Brilho (n)",    &sel.material.shininess, 1.0f, 256.0f);
        }
    } else {
        // Mensagem de aviso quando nenhum modelo foi passado como argumento
        ImGui::TextColored({1,0.4f,0.4f,1}, "Nenhum modelo carregado.");
        ImGui::TextWrapped("Passe arquivos .obj/.ply como argumento:\nvisualizar modelo.obj");
    }

    ImGui::End();
}
