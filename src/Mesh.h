#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

#include "Shader.h"

// Representa uma malha 3D geométrica e gerencia seus buffers na GPU.
// Cada Mesh possui um VAO (Vertex Array Object) que agrupa as configurações de atributos,
// um VBO (Vertex Buffer Object) com os dados de vértices e um EBO (Element Buffer Object)
// com os índices de triângulos. Após o construtor, os dados já residem na GPU.

// Estrutura de um único vértice, espelhando o layout esperado pelos vertex shaders.
// Os campos são mapeados para os atributos de entrada do shader pelas locations abaixo:
struct Vertex {
    glm::vec3 Position;  // location 0 — posição 3D no espaço do modelo
    glm::vec3 Normal;    // location 1 — normal da superfície (usada no cálculo de iluminação)
    glm::vec2 TexCoords; // location 2 — coordenadas de textura UV (não usadas neste projeto, mas mantidas para compatibilidade)
};


class Mesh
{
public:
    std::vector<Vertex>       vertices; // Cópia dos vértices na CPU (usada para debug/inspeção)
    std::vector<unsigned int> indices;  // Índices dos triângulos; cada grupo de 3 forma uma face

    // Identificadores OpenGL dos buffers alocados na GPU
    unsigned int VAO, VBO, EBO;

    // Constrói a malha, transferindo os dados para a GPU via setupMesh().
    // Os vetores são movidos para evitar cópia desnecessária.
    Mesh(std::vector<Vertex> verts, std::vector<unsigned int> idx)
        : vertices(std::move(verts)), indices(std::move(idx))
    {
        setupMesh();
    }

    // Desenha a malha usando o VAO configurado em setupMesh().
    // glDrawElements emite um draw call com triângulos indexados pelo EBO.
    void draw() const
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0); // Desvincula o VAO para não afetar estados seguintes
    }

    // Libera os recursos de GPU alocados para esta malha.
    // Deve ser chamado antes de destruir o objeto para evitar vazamento de memória na GPU.
    void free()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

private:
    // Gera e configura os objetos de buffer na GPU.
    // O VAO guarda o estado completo dos atributos, permitindo redesenhar com um único bind.
    void setupMesh()
    {
        // Gera os identificadores para VAO, VBO e EBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Toda configuração de atributos feita enquanto o VAO está vinculado fica gravada nele
        glBindVertexArray(VAO);

        // Envia o array de vértices para a GPU como dado estático (não muda após o upload)
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(Vertex),
                     vertices.data(),
                     GL_STATIC_DRAW);

        // Envia os índices para a GPU; o EBO fica associado ao VAO atual
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned int),
                     indices.data(),
                     GL_STATIC_DRAW);

        // Configura os ponteiros de atributos — diz ao OpenGL onde, dentro do VBO,
        // estão cada campo do struct Vertex. offsetof calcula o byte offset de cada membro.

        // location 0 → Position: 3 floats, sem normalização, stride = tamanho total do Vertex
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, Position));

        // location 1 → Normal: 3 floats
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, Normal));

        // location 2 → TexCoords: 2 floats
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0); // Desvincula para evitar modificações acidentais
    }
};
