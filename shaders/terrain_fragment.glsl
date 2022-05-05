#version 460 core

layout(location = 0) in vec3 o_normals;
layout(location = 1) in vec3 o_position;
layout(location = 2) in vec2 o_texture;
layout(location = 3) in vec3 o_cameraPos;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D u_textureSampler;

layout(std140, binding = 1) uniform Env
{
    vec4 ambient;
    vec3 toLight;
    vec3 sunColor;
} env;

void main()
{
    float brightness = max(dot(o_normals, env.toLight), 0.0) * 0.9;
    vec3 lightColor = env.sunColor * brightness;

    vec3 ambientColor = env.ambient.xyz * env.ambient.w;

    vec4 cleanColor = vec4(ambientColor + lightColor, 1.0)
                    * texture(u_textureSampler, o_texture);

    float dist = distance(o_position, o_cameraPos);
    float farDist = 64.0;
    float ratio = min(dist / farDist, 1.0);
    o_color = (1.0 - ratio) * cleanColor
            + ratio         * vec4(ambientColor, 1.0);
}
