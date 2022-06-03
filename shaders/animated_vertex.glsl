#version 460 core

layout(location = 0) in vec3  i_position;
layout(location = 1) in vec3  i_normal;
layout(location = 2) in vec2  i_texture;
layout(location = 3) in ivec4 i_ids;
layout(location = 4) in vec4  i_weights;

layout(location = 0) out vec3 o_normal;
layout(location = 1) out vec3 o_position;
layout(location = 2) out vec2 o_texture;
layout(location = 3) out vec3 o_cameraPos;

layout(location = 0) uniform mat4 u_jointMatrices[64];

layout(std140, binding = 0) uniform Cam
{
    mat4 viewMat;
    mat4 projMat;
    mat4 vpMat;
    vec3 pos;
} cam;

void main(void)
{
    vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 normal   = vec4(0.0, 0.0, 0.0, 0.0);

    for (int i = 0; i < 4; ++i) {
        mat4 jointMatrix = u_jointMatrices[i_ids[i]];
        float weight = i_weights[i];
        position += (jointMatrix * vec4(i_position, 1.0)) * weight;
        normal   += (jointMatrix * vec4(i_normal,   0.0)) * weight;
    }

    gl_Position = cam.vpMat * position;
    o_normal = normalize(normal.xyz);
    o_position = position.xyz;
    o_texture = i_texture;
    o_cameraPos = cam.pos;
}
