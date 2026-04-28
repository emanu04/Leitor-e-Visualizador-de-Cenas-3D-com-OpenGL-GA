#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// movimento é controlado pelos métodos processKeyboard() e processMouse().

// movimento possíveis
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class Camera
{
public:
    // Posição e orientação
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Ângulos de Euler (rotação da câmera)
    float Yaw;   // rotação horizontal (eixo Y) yaw
    float Pitch; // rotação vertical   (eixo X) pitch

    // Parâmetros de controle
    float MovementSpeed;
    float MouseSensitivity;
    float Fov;         

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
        updateCameraVectors();
    }

    // Retorna a matriz de visão
    //glm::lookAt(posicao, posicao + frente, up) cria a matriz que posiciona a camera
    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Processa entrada de teclado para mover a câmera
    void processKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)  Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT)     Position -= Right * velocity;
        if (direction == RIGHT)    Position += Right * velocity;
        if (direction == UP)       Position += WorldUp * velocity;
        if (direction == DOWN)     Position -= WorldUp * velocity;
    }

    // Processa movimento do mouse para rotacionar a câmera
    void processMouse(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // evita inversão de camera
        if (constrainPitch) {
            if (Pitch >  89.0f) Pitch =  89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    //zoom com scroll do mouse
    void processScroll(float yoffset)
    {
        Fov -= yoffset;
        if (Fov < 1.0f)  Fov = 1.0f;
        if (Fov > 90.0f) Fov = 90.0f;
    }

private:
    //recalcula a partir de angulos de euler
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
