#version 330 core

out vec4 FragColor;

uniform vec3 wireColor;

void main()
{
    FragColor = vec4(wireColor, 1.0);
}
