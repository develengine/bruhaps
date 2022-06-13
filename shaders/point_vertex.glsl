#version 460 core

layout(location = 0) out vec4 o_color;

layout(location = 0) uniform mat4 u_modMat;
layout(location = 1) uniform vec4 u_colors[64];
layout(location = 65) uniform vec4 u_positions[64];

layout(std140, binding = 0) uniform Cam
{
    mat4 viewMat;
    mat4 projMat;
    mat4 vpMat;
    vec3 pos;
} cam;

void main(void)
{
    o_color = u_colors[gl_InstanceID];
    gl_PointSize = u_positions[gl_InstanceID].w;
    gl_Position = cam.vpMat * u_modMat * vec4(u_positions[gl_InstanceID].xyz, 1.0);
}
