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
layout(location = 1) uniform vec3 u_transform;

void main(void)
{
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    /*
    vector = normalize((transpose(mat3(u_view)) * -1)
           * (vec3(-positions[gl_VertexID], 1.0) * u_transform));
           */
    vector = (u_view * vec4(vec3(positions[gl_VertexID], 1.0) * normalize(u_transform), 0.0)).xyz;
}
