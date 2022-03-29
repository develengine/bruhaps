#version 460 core

const vec2 positions[6] = {
    {-1.0, 1.0 },
    {-1.0,-1.0 },
    { 1.0,-1.0 },

    { 1.0,-1.0 },
    { 1.0, 1.0 },
    {-1.0, 1.0 },
};

layout(location = 0) out vec3 vector;

layout(location = 0) uniform mat4 u_view;

void main(void)
{
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    vector = (u_view * vec4(positions[gl_VertexID].x, positions[gl_VertexID].y, 1.0, 0.0)).xyz;
}