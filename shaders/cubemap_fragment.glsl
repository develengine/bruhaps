#version 460 core

layout(location = 0) in vec3 vector;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform samplerCube environment;

layout(std140, binding = 1) uniform Env
{
    vec4 ambient;
    vec3 toLight;
    vec3 sunColor;
} env;

void main(void)
{
    outColor = texture(environment, vector) * vec4(env.sunColor, 1.0);
}
