#version 460 core

layout(location = 0) out vec4 o_color;

layout(location = 1) uniform vec4 u_colors[32];
layout(location = 33) uniform vec4 u_positions[32];

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
    gl_Position = cam.vpMat * vec4(u_positions[gl_InstanceID].xyz, 1.0);
}
