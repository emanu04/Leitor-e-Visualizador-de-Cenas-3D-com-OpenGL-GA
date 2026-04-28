#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Material properties
uniform vec3 ka;  // ambient
uniform vec3 kd;  // diffuse
uniform vec3 ks;  // specular
uniform float shininess;

void main()
{
    // Ambient
    vec3 ambient = ka * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * kd * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = spec * ks * lightColor;
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
