#version 460 core

layout(location = 0) in vec3 o_normal;
layout(location = 1) in vec3 o_position;
layout(location = 2) in vec2 o_texture;

layout(location = 0) out vec4 o_color;

layout(location = 1) uniform vec3 u_cameraPos;
layout(location = 2) uniform vec3 u_objColor;

layout(binding = 0) uniform sampler2D u_textureSampler;

void main(void)
{
    vec3 toLight = vec3(0.6, 0.7, 0.5);

    float brightness = max(dot(o_normal, toLight), 0.0) * 0.9;
    vec3 lightColor = vec3(1.0, 1.0, 1.0) * brightness;

    float ambientPower = 0.4;
    vec3 ambientColor = vec3(0.1, 0.2, 0.3) * ambientPower;

    vec3 toCamera = normalize(u_cameraPos - o_position);
    vec3 reflection = normalize(reflect(-toLight, o_normal));
    float shine = pow(max(dot(toCamera, reflection), 0.0), 32);
    vec3 specular = shine * 0.5 * lightColor;

    o_color = vec4(ambientColor + lightColor + specular, 1.0)
            * texture(u_textureSampler, o_texture);
            // * vec4(u_objColor, 1.0);
}

