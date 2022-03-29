#version 460 core

layout(location = 0) in vec3 vector;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform samplerCube environment;

void main(void)
{
    outColor = texture(environment, vector);
}