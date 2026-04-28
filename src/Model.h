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


struct PhongMaterial {
    glm::vec3 ka        = {0.1f, 0.1f, 0.1f}; // ambiente
    glm::vec3 kd        = {0.7f, 0.7f, 0.7f}; // difuso
    glm::vec3 ks        = {0.5f, 0.5f, 0.5f}; // especular
    float     shininess = 32.0f;               // brilho especular
};


//carrega arquivo 3d
class Model
{
public:
    std::string        name;      // nome de exibição
    std::vector<Mesh>  meshes;    // lista de malhas extraídas do arquivo
    PhongMaterial      material;  // propriedades de material configuráveis

    // Transformações no espaço mundo
    glm::vec3 position    = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation    = {0.0f, 0.0f, 0.0f}; // ângulos em graus (X, Y, Z)
    glm::vec3 scale       = {1.0f, 1.0f, 1.0f};

    explicit Model(const std::string& path)
    {
        size_t sep = path.find_last_of("/\\");
        name = (sep != std::string::npos) ? path.substr(sep + 1) : path;

        loadModel(path);
    }

    // Retorna a matriz de modelo combinando translação, rotação e escala
    glm::mat4 getModelMatrix() const
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.x), {1, 0, 0});
        m = glm::rotate(m, glm::radians(rotation.y), {0, 1, 0});
        m = glm::rotate(m, glm::radians(rotation.z), {0, 0, 1});
        m = glm::scale(m, scale);
        return m;
    }

    // Desenha todas as malhas do modelo aplicando os uniformes de material
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

    void free()
    {
        for (auto& mesh : meshes)
            mesh.free();
    }

private:
    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;


        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "[Assimp] Erro ao carregar " << path << ": "
                      << importer.GetErrorString() << "\n";
            return;
        }

        processNode(scene->mRootNode, scene);
    }

    // Percorre a hierarquia de nós do Assimp recursivamente
    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    // Converte uma aiMesh para nossa estrutura Mesh (CPU → GPU via setupMesh)
    Mesh processMesh(aiMesh* mesh, const aiScene* /*scene*/)
    {
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            // Posição
            vertex.Position = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            };

            if (mesh->HasNormals()) {
                vertex.Normal = {
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                };
            } else {
                vertex.Normal = {0.0f, 1.0f, 0.0f};
            }

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

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        return Mesh(std::move(vertices), std::move(indices));
    }
};
