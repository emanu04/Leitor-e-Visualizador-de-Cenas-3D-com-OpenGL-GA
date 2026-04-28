#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
#include <iostream>

#include "Mesh.h"
#include "Shader.h"

// Carrega um arquivo 3D do disco (via Assimp) e o representa como uma coleção de malhas.
// Também mantém as transformações (posição, rotação, escala) e o material Phong do objeto.

// Coeficientes do modelo de iluminação de Phong.
// A equação é: cor = ka*Ia + kd*(L·N)*Id + ks*(R·V)^n*Is
//   ka: reflexão ambiente — cor base mesmo sem luz direta
//   kd: reflexão difusa  — cor dependente do ângulo entre a normal e a luz
//   ks: reflexão especular — brilho highlight dependente do ângulo de visão
//   shininess: expoente n, quanto maior mais concentrado e brilhante o highlight
struct PhongMaterial {
    glm::vec3 ka        = {0.1f, 0.1f, 0.1f}; // coeficiente ambiente (valor baixo para não "lavar" a cena)
    glm::vec3 kd        = {0.7f, 0.7f, 0.7f}; // coeficiente difuso (cor principal do objeto)
    glm::vec3 ks        = {0.5f, 0.5f, 0.5f}; // coeficiente especular (intensidade do brilho)
    float     shininess = 32.0f;               // expoente especular (32 = brilho moderado)
};


// Representa um modelo 3D completo: conjunto de malhas + material + transformações no espaço mundo.
// Usa Assimp para parsear formatos como .obj e .ply, convertendo para nossa estrutura Mesh/Vertex.
class Model
{
public:
    std::string        name;      // Nome de exibição extraído do caminho do arquivo
    std::vector<Mesh>  meshes;    // Lista de malhas; um arquivo pode ter várias sub-meshes
    PhongMaterial      material;  // Propriedades de material editáveis em tempo de execução

    // Transformações no espaço mundo aplicadas na ordem: translação → rotação → escala
    glm::vec3 position    = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation    = {0.0f, 0.0f, 0.0f}; // ângulos em graus (X, Y, Z)
    glm::vec3 scale       = {1.0f, 1.0f, 1.0f};

    // Constrói o modelo carregando o arquivo indicado por path.
    // Extrai o nome de exibição do último componente do caminho (após '/' ou '\').
    explicit Model(const std::string& path)
    {
        size_t sep = path.find_last_of("/\\");
        name = (sep != std::string::npos) ? path.substr(sep + 1) : path;

        loadModel(path);
    }

    // Constrói e retorna a matriz de modelo (Model Matrix) combinando as transformações.
    // A ordem de multiplicação é importante: T * Rx * Ry * Rz * S
    // (a escala é aplicada primeiro no espaço do objeto, depois as rotações, depois a translação)
    glm::mat4 getModelMatrix() const
    {
        glm::mat4 m = glm::mat4(1.0f); // começa com a identidade
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.x), {1, 0, 0});
        m = glm::rotate(m, glm::radians(rotation.y), {0, 1, 0});
        m = glm::rotate(m, glm::radians(rotation.z), {0, 0, 1});
        m = glm::scale(m, scale);
        return m;
    }

    // Envia os uniformes de material e a model matrix para o shader e chama draw() em cada malha.
    // O shader precisa ter os uniformes: model (mat4), ka, kd, ks (vec3) e shininess (float).
    void draw(Shader& shader) const
    {
        shader.setMat4("model", getModelMatrix());
        shader.setVec3("ka",        material.ka);
        shader.setVec3("kd",        material.kd);
        shader.setVec3("ks",        material.ks);
        shader.setFloat("shininess", material.shininess);

        for (const auto& mesh : meshes)
            mesh.draw();
    }

    // Libera os buffers de GPU de todas as malhas. Deve ser chamado antes de destruir o modelo.
    void free()
    {
        for (auto& mesh : meshes)
            mesh.free();
    }

private:
    // Abre o arquivo com Assimp e inicia o processamento da hierarquia de nós.
    // Flags de pós-processamento:
    //   aiProcess_Triangulate     — converte polígonos (quads, etc.) em triângulos
    //   aiProcess_GenSmoothNormals — gera normais suavizadas se o arquivo não as tiver
    //   aiProcess_FlipUVs         — inverte o eixo V das UVs (OpenGL usa Y-up para texturas)
    //   aiProcess_CalcTangentSpace — calcula tangentes/bitangentes (útil para normal mapping)
    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

        // Verifica se o arquivo foi carregado e a cena está válida
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "[Assimp] Erro ao carregar " << path << ": "
                      << importer.GetErrorString() << "\n";
            return;
        }

        // Percorre a árvore de nós da cena a partir da raiz
        processNode(scene->mRootNode, scene);
    }

    // Percorre a hierarquia de nós do Assimp recursivamente.
    // Cada nó pode referenciar múltiplas malhas e ter filhos (sub-objetos), formando uma árvore.
    void processNode(aiNode* node, const aiScene* scene)
    {
        // Processa todas as malhas deste nó
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // Processa os nós filhos recursivamente
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    // Converte uma aiMesh do Assimp para nossa estrutura Mesh (CPU → GPU via setupMesh).
    // Extrai posições, normais e coordenadas de textura, com fallbacks para dados ausentes.
    Mesh processMesh(aiMesh* mesh, const aiScene* /*scene*/)
    {
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            // Posição XYZ do vértice no espaço do modelo
            vertex.Position = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            };

            // Normal da superfície — se ausente, usa (0,1,0) como fallback (aponta para cima)
            if (mesh->HasNormals()) {
                vertex.Normal = {
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                };
            } else {
                vertex.Normal = {0.0f, 1.0f, 0.0f};
            }

            // Coordenadas de textura UV — Assimp suporta até 8 canais; usamos o canal 0.
            // Se o arquivo não tiver UVs, usa (0,0) como fallback.
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = {
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                };
            } else {
                vertex.TexCoords = {0.0f, 0.0f};
            }

            vertices.push_back(vertex);
        }

        // Extrai os índices de cada face (já trianguladas pelo aiProcess_Triangulate)
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        return Mesh(std::move(vertices), std::move(indices));
    }
};
