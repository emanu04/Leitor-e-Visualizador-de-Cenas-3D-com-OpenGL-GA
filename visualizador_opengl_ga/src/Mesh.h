#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

#include "Shader.h"

// ---------------------------------------------------------------------------
// Vertex
// Estrutura de dados que representa um único vértice na memória da CPU.
// O layout (posição → normal → textura) deve corresponder exatamente ao
// glVertexAttribPointer configurado em setupMesh().
// ---------------------------------------------------------------------------
struct Vertex {
    glm::vec3 Position;  // location 0
    glm::vec3 Normal;    // location 1
    glm::vec2 TexCoords; // location 2
};

// ---------------------------------------------------------------------------
// Mesh
// Encapsula o VAO, VBO e EBO de uma malha triangular.
// Recebe vértices e índices já processados pelo Model/parser.
// ---------------------------------------------------------------------------
class Mesh
{
public:
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO, VBO, EBO;

    Mesh(std::vector<Vertex> verts, std::vector<unsigned int> idx)
        : vertices(std::move(verts)), indices(std::move(idx))
    {
        setupMesh();
    }

    // Desenha a malha usando o shader já ativo (use() deve ter sido chamado antes)
    void draw() const
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Libera recursos de GPU
    void free()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

private:
    void setupMesh()
    {
        // Gera os objetos de buffer
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Vincula o VAO para gravar o estado de atributos
        glBindVertexArray(VAO);

        // Envia dados de vértices para a GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(Vertex),
                     vertices.data(),
                     GL_STATIC_DRAW);

        // Envia índices (EBO) para a GPU
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned int),
                     indices.data(),
                     GL_STATIC_DRAW);

        // Configura os ponteiros de atributos:
        // location 0 → Position (3 floats)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, Position));

        // location 1 → Normal (3 floats)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, Normal));

        // location 2 → TexCoords (2 floats)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};
