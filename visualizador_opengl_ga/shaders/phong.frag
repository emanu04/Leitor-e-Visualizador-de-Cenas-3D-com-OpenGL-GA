#version 330 core

// ----- Entradas interpoladas do Vertex Shader -----
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// ----- Saída de cor -----
out vec4 FragColor;

// ----- Fonte de luz pontual -----
uniform vec3 lightPos;      // posição da luz no espaço mundo
uniform vec3 lightColor;    // cor/intensidade da luz

// ----- Propriedades do material (refletância) -----
uniform vec3  ka;           // coeficiente ambiente
uniform vec3  kd;           // coeficiente difuso
uniform vec3  ks;           // coeficiente especular
uniform float shininess;    // brilho especular (expoente de Phong)

// ----- Posição da câmera (para calcular vetor de visão) -----
uniform vec3 viewPos;

void main()
{
    // --- Componente Ambiente ---
    vec3 ambient = ka * lightColor;

    // --- Componente Difusa ---
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = kd * diff * lightColor;

    // --- Componente Especular (modelo de Phong) ---
    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);         // vetor de reflexão
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular   = ks * spec * lightColor;

    // --- Cor final ---
    vec3 result = ambient + diffuse + specular;
    FragColor   = vec4(result, 1.0);
}
