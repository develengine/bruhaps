#version 460 core

layout(location = 0) in vec3 i_position;

layout(location = 0) out vec3 vector;

layout(location = 0) uniform mat4 u_view;
layout(location = 1) uniform mat4 u_projection;

void main(void)
{
    gl_Position = (u_projection * vec4(mat3(u_view) * i_position, 1.0));
    /*
    vector = normalize((transpose(mat3(u_view)) * -1)
           * (vec3(-positions[gl_VertexID], 1.0) * u_transform));
           */
    // vector = (u_view * vec4(vec3(positions[gl_VertexID], 1.0) * normalize(u_transform), 0.0)).xyz;
    vector = i_position;
    // vector = vec3(u_view * vec4(i_position, 0.0));
}
