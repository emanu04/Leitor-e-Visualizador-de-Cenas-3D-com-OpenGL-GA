#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// Encapsula a compilação, linkagem e uso de um programa de shader OpenGL (GLSL).
// Fluxo de criação:
//   1. readFile()           — lê o código GLSL dos arquivos .vert e .frag do disco
//   2. glCompileShader()    — compila cada shader individualmente na GPU
//   3. glLinkProgram()      — linka vertex + fragment em um único programa executável
//   4. use()                — ativa o programa antes de enviar uniformes e desenhar
// Os métodos set*() são atalhos para enviar valores às variáveis uniform dos shaders.


class Shader
{
public:
    unsigned int ID; // Identificador do programa OpenGL (retornado por glCreateProgram)

    // Carrega, compila e linka os shaders a partir dos caminhos de arquivo fornecidos.
    // O programa compilado fica disponível no atributo ID após o construtor retornar.
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. Lê o código-fonte GLSL dos arquivos .vert e .frag do disco
        std::string   vertexCode   = readFile(vertexPath);
        std::string   fragmentCode = readFile(fragmentPath);
        const char*   vShaderCode  = vertexCode.c_str();
        const char*   fShaderCode  = fragmentCode.c_str();

        // 2. Compila o Vertex Shader — executa uma vez por vértice, transforma posições
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX", vertexPath);

        // 3. Compila o Fragment Shader — executa uma vez por fragmento (pixel), calcula a cor final
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT", fragmentPath);

        // 4. Linka os dois shaders em um único programa executável na GPU
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM", "");

        // Os objetos de shader individuais não são mais necessários após a linkagem
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // Ativa este programa de shader para uso nos draw calls seguintes
    void use() const { glUseProgram(ID); }

    // Envia um booleano para um uniform do tipo int no shader (OpenGL não tem uniform bool nativo)
    void setBool (const std::string& name, bool  value) const
    { glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); }

    // Envia um inteiro para um uniform int no shader
    void setInt  (const std::string& name, int   value) const
    { glUniform1i(glGetUniformLocation(ID, name.c_str()), value); }

    // Envia um float para um uniform float no shader
    void setFloat(const std::string& name, float value) const
    { glUniform1f(glGetUniformLocation(ID, name.c_str()), value); }

    // Envia um vetor de 3 floats (glm::vec3) para um uniform vec3 no shader
    void setVec3 (const std::string& name, const glm::vec3& value) const
    { glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value)); }

    // Sobrecarga: envia três floats separados para um uniform vec3 no shader
    void setVec3 (const std::string& name, float x, float y, float z) const
    { glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); }

    // Envia uma matriz 4x4 para um uniform mat4 no shader.
    // GL_FALSE indica que a matriz NÃO deve ser transposta (glm já usa column-major como OpenGL)
    void setMat4 (const std::string& name, const glm::mat4& mat) const
    { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }

private:
    // Lê o conteúdo de um arquivo de texto e retorna como string.
    // Usa exceções de ifstream para detectar falhas de IO sem checar errno manualmente.
    static std::string readFile(const char* path)
    {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path);
            std::stringstream ss;
            ss << file.rdbuf(); // lê o buffer inteiro de uma vez
            file.close();
            return ss.str();
        } catch (std::ifstream::failure& e) {
            std::cerr << "[Shader] Erro ao ler arquivo: " << path << "\n";
            return "";
        }
    }

    // Verifica erros de compilação (shaders) ou de linkagem (programa).
    // O parâmetro type distingue os dois casos, pois as funções de consulta são diferentes:
    //   glGetShaderiv  + glGetShaderInfoLog  — para shaders individuais
    //   glGetProgramiv + glGetProgramInfoLog — para o programa linkado
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
