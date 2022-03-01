#version 460 core

layout(location = 0) in vec3 position;

out V_OUT
{
    vec4 color;
} frag;

layout(location = 0) uniform mat4 u_mvp;
layout(location = 1) uniform vec4 u_color;

void main()
{
    frag.color  = u_color;
    gl_Position = u_mvp * vec4(position, 1.0);
}
