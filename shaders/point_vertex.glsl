#version 460 core

layout(location = 0) uniform mat4 u_vpMat;
layout(location = 1) uniform vec4 u_position;

void main(void)
{
    gl_Position = u_vpMat * u_position;
}
