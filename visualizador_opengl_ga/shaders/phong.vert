#version 330 core

// ----- Atributos de entrada (layout deve bater com os VBOs) -----
layout(location = 0) in vec3 aPos;      // posição do vértice
layout(location = 1) in vec3 aNormal;   // normal do vértice
layout(location = 2) in vec2 aTexCoord; // coordenada de textura

// ----- Uniformes de transformação -----
uniform mat4 model;      // matriz de modelo (objeto → mundo)
uniform mat4 view;       // matriz de visão  (mundo  → câmera)
uniform mat4 projection; // matriz de projeção

// ----- Saídas para o Fragment Shader -----
out vec3 FragPos;    // posição do fragmento no espaço mundo
out vec3 Normal;     // normal interpolada no espaço mundo
out vec2 TexCoord;   // coordenada de textura

void main()
{
    // Posição do vértice no espaço mundo
    FragPos   = vec3(model * vec4(aPos, 1.0));

    // Normal transformada pela matriz normal (inversa-transposta do model)
    // Isso corrige normais quando há escala não-uniforme
    Normal    = mat3(transpose(inverse(model))) * aNormal;

    TexCoord  = aTexCoord;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
