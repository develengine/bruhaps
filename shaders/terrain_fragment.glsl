#version 460 core

layout(location = 0) in vec3 o_normals;
layout(location = 1) in vec3 o_position;
layout(location = 2) in vec2 o_texture;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D u_textureSampler;

void main()
{
    vec3 toLight = vec3(0.6, 0.7, 0.5);

    float brightness = max(dot(o_normals, toLight), 0.0) * 0.9;
    vec3 lightColor = vec3(1.0, 1.0, 1.0) * brightness;

    float ambientPower = 1.0;
    vec3 ambientColor = vec3(0.1, 0.2, 0.3) * ambientPower;

    o_color = vec4(ambientColor + lightColor, 1.0)
            * texture(u_textureSampler, o_texture);
}
