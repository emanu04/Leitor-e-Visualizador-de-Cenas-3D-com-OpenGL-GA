#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// ---------------------------------------------------------------------------
// Shader
// Carrega, compila e linka um par de shaders (vertex + fragment).
// Expõe métodos utilitários para configurar uniformes.
// ---------------------------------------------------------------------------
class Shader
{
public:
    unsigned int ID; // ID do programa OpenGL

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. Lê os arquivos de shader do disco
        std::string   vertexCode   = readFile(vertexPath);
        std::string   fragmentCode = readFile(fragmentPath);
        const char*   vShaderCode  = vertexCode.c_str();
        const char*   fShaderCode  = fragmentCode.c_str();

        // 2. Compila o Vertex Shader
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX", vertexPath);

        // 3. Compila o Fragment Shader
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT", fragmentPath);

        // 4. Linka o programa de shaders
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM", "");

        // Após linkagem, os shaders individuais não são mais necessários
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() const { glUseProgram(ID); }

    // ----- Uniformes -----
    void setBool (const std::string& name, bool  value) const
    { glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); }

    void setInt  (const std::string& name, int   value) const
    { glUniform1i(glGetUniformLocation(ID, name.c_str()), value); }

    void setFloat(const std::string& name, float value) const
    { glUniform1f(glGetUniformLocation(ID, name.c_str()), value); }

    void setVec3 (const std::string& name, const glm::vec3& value) const
    { glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value)); }

    void setVec3 (const std::string& name, float x, float y, float z) const
    { glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); }

    void setMat4 (const std::string& name, const glm::mat4& mat) const
    { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }

private:
    static std::string readFile(const char* path)
    {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path);
            std::stringstream ss;
            ss << file.rdbuf();
            file.close();
            return ss.str();
        } catch (std::ifstream::failure& e) {
            std::cerr << "[Shader] Erro ao ler arquivo: " << path << "\n";
            return "";
        }
    }

    static void checkCompileErrors(unsigned int shader, const std::string& type, const std::string& name)
    {
        int  success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "[Shader] Erro de compilação (" << type << ") em " << name << ":\n"
                          << infoLog << "\n";
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "[Shader] Erro de linkagem:\n" << infoLog << "\n";
            }
        }
    }
};
