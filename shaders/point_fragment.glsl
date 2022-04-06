#version 460 core

layout(location = 0) out vec4 o_color;

layout(location = 2) uniform vec4 u_color;

void main(void)
{
    o_color = u_color;
}
