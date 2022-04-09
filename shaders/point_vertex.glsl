#version 460 core

layout(location = 0) uniform mat4 u_vpMat;
layout(location = 1) uniform vec4 u_position;
layout(location = 3) uniform float u_size;

void main(void)
{
    gl_PointSize = u_size;
    gl_Position = u_vpMat * u_position;
}
