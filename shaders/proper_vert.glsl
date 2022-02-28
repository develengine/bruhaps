#version 460 core

layout(location = 0) in vec3 position;

out V_OUT
{
    vec4 color;
} frag;

layout(location = 0) uniform vec3 u_position;
layout(location = 1) uniform vec3 u_size;
layout(location = 2) uniform vec4 u_color;

void main()
{
    frag.color  = u_color;
    gl_Position = vec4(position * u_size + u_position, 1.0);
}
