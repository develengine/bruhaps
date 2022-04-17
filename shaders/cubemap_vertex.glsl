#version 460 core

layout(location = 0) in vec3 i_position;

layout(location = 0) out vec3 vector;

layout(std140, binding = 0) uniform Cam
{
    mat4 viewMat;
    mat4 projMat;
    mat4 vpMat;
    vec3 pos;
} cam;

void main(void)
{
    gl_Position = (cam.projMat * vec4(mat3(cam.viewMat) * i_position, 1.0));
    vector = i_position;
}
