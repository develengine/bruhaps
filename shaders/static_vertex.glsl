#version 460 core

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec2 i_textures;
layout(location = 2) in vec3 i_normals;

layout(location = 0) out vec3 o_normals;
layout(location = 1) out vec3 o_position;
layout(location = 2) out vec2 o_textures;
layout(location = 3) out vec3 o_cameraPos;

layout(location = 0) uniform uint u_modOffset;

layout(std140, binding = 0) uniform Cam
{
    mat4 viewMat;
    mat4 projMat;
    mat4 vpMat;
    vec3 pos;
} cam;

layout(std140, binding = 2) uniform Stats
{
    mat4 modMats[1024];
} stats;

void main() {
    mat4 modMat = stats.modMats[u_modOffset + gl_InstanceID];

    vec4 position = modMat * vec4(i_position, 1.0);
    gl_Position = cam.vpMat * position;

    o_normals = mat3(transpose(inverse(modMat))) * i_normals;
    o_position = position.xyz;
    o_textures = i_textures;

    o_cameraPos = cam.pos;
}
