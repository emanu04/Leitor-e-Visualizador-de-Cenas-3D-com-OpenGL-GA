#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Câmera no estilo FPS (First-Person Shooter).
// A orientação é descrita por dois ângulos de Euler: Yaw (horizontal) e Pitch (vertical).
// A partir desses ângulos, os vetores Front, Right e Up são recalculados a cada atualização.
// O movimento é controlado pelos métodos processKeyboard() e processMouse().

// Direções possíveis de movimento da câmera
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class Camera
{
public:
    // Posição atual da câmera no espaço mundo
    glm::vec3 Position;
    // Vetor unitário apontando para onde a câmera está olhando
    glm::vec3 Front;
    // Vetor "cima" local da câmera (recalculado com cross product)
    glm::vec3 Up;
    // Vetor "direita" da câmera (perpendicular a Front e WorldUp)
    glm::vec3 Right;
    // Vetor "cima" global fixo, usado como referência para recalcular Right e Up
    glm::vec3 WorldUp;

    // Ângulos de Euler que definem a orientação da câmera
    float Yaw;   // Rotação horizontal em torno do eixo Y (em graus)
    float Pitch; // Rotação vertical em torno do eixo X (em graus)

    // Parâmetros de controle de movimento e sensibilidade
    float MovementSpeed;    // Unidades por segundo que a câmera se move
    float MouseSensitivity; // Fator de escala do deslocamento do mouse
    float Fov;              // Campo de visão (field of view) em graus, usado na projeção perspectiva

    // Constrói a câmera com posição inicial e orientação padrão.
    // Yaw=-90 faz a câmera olhar inicialmente para -Z (convenção OpenGL com Z saindo da tela).
    Camera(glm::vec3 position = {0.0f, 0.0f, 3.0f},
           glm::vec3 up       = {0.0f, 1.0f, 0.0f},
           float yaw          = -90.0f,
           float pitch        = 0.0f)
        : Front({0.0f, 0.0f, -1.0f}),
          MovementSpeed(5.0f),
          MouseSensitivity(0.1f),
          Fov(45.0f)
    {
        Position = position;
        WorldUp  = up;
        Yaw      = yaw;
        Pitch    = pitch;
        // Calcula Front, Right e Up a partir dos ângulos iniciais
        updateCameraVectors();
    }

    // Retorna a matriz de visão (view matrix) que transforma coordenadas do espaço mundo
    // para o espaço da câmera. glm::lookAt coloca a câmera em Position olhando para Position+Front.
    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Move a câmera com base na tecla pressionada.
    // A velocidade é escalada por deltaTime para movimento independente do frame rate.
    void processKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)  Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT)     Position -= Right * velocity;
        if (direction == RIGHT)    Position += Right * velocity;
        // UP/DOWN usam WorldUp para subir/descer verticalmente sem depender da inclinação da câmera
        if (direction == UP)       Position += WorldUp * velocity;
        if (direction == DOWN)     Position -= WorldUp * velocity;
    }

    // Atualiza Yaw e Pitch com base no deslocamento do mouse e recalcula os vetores de orientação.
    // xoffset: movimento horizontal do mouse; yoffset: movimento vertical (já invertido pelo chamador).
    void processMouse(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // Limita o pitch a ±89° para evitar que a câmera "vire de cabeça para baixo"
        // (ultrapassar 90° inverteria o vetor Up e causaria comportamento inesperado)
        if (constrainPitch) {
            if (Pitch >  89.0f) Pitch =  89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    // Ajusta o campo de visão (zoom) com base no scroll do mouse.
    // Diminuir Fov aproxima a imagem; aumentar afasta. Limitado entre 1° e 90°.
    void processScroll(float yoffset)
    {
        Fov -= yoffset;
        if (Fov < 1.0f)  Fov = 1.0f;
        if (Fov > 90.0f) Fov = 90.0f;
    }

private:
    // Recalcula os vetores Front, Right e Up a partir dos ângulos de Euler (Yaw e Pitch).
    // As fórmulas usam trigonometria esférica: Yaw rotaciona no plano XZ, Pitch inclina em Y.
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // Right é perpendicular a Front e ao "cima" global (produto vetorial)
        Right = glm::normalize(glm::cross(Front, WorldUp));
        // Up local é perpendicular a Right e Front, garantindo base ortonormal
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
