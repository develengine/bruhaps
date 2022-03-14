#version 460 core

layout(location = 0) in vec3 o_normals;
layout(location = 1) in vec3 o_position;
layout(location = 2) in vec3 o_textures;

layout(location = 0) out vec4 o_color;

layout(location = 2) uniform vec3 u_cameraPos;
layout(location = 3) uniform vec3 u_objColor;

layout(location = 4) uniform sampler2D textureSampler;

void main() {
    vec3 toLight = vec3(0.6, 0.7, 0.5);

    float brightness = max(dot(o_normals, toLight), 0.0) * 0.9;
    vec3 lightColor = vec3(1.0, 1.0, 1.0) * brightness;

    float ambientPower = 0.4;
    vec3 ambientColor = vec3(0.1, 0.2, 0.3) * ambientPower;

    vec3 toCamera = normalize(u_cameraPos - o_position);
    vec3 reflection = normalize(reflect(-toLight, o_normals));
    float shine = pow(max(dot(toCamera, reflection), 0.0), 32);
    vec3 specular = shine * 0.5 * lightColor;

    o_color = vec4(ambientColor + lightColor + specular, 1.0)
            * vec4(u_objColor, 1.0);
}

