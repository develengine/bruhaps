#version 460 core

layout(location = 0) in vec3 o_normals;
layout(location = 1) in vec3 o_position;
layout(location = 2) in vec2 o_textures;
layout(location = 3) in vec3 o_cameraPos;

layout(location = 0) out vec4 o_color;

layout(location = 1) uniform vec4 u_lightPos;
layout(location = 2) uniform vec4 u_lightCol;

layout(binding = 0) uniform sampler2D textureSampler;

layout(std140, binding = 1) uniform Env
{
    vec4 ambient;
    vec3 toLight;
    vec3 sunColor;
} env;

void main()
{
    vec3 normal = normalize(o_normals);
    vec3 objColor = texture(textureSampler, o_textures).rgb;

    vec3 toLight2 = normalize(u_lightPos.xyz - o_position);
    float lightDistance2 = distance(o_position, u_lightPos.xyz);

    float brightness = max(dot(normal, env.toLight), 0.0) * 0.9;
    vec3 lightColor = env.sunColor * brightness;

    float brightness2 = max(dot(normal, toLight2), 0.0) * 0.9 * (1 / lightDistance2);
    vec3 lightColor2 = u_lightCol.rgb * brightness2;

    vec3 ambientColor = env.ambient.xyz * env.ambient.w;

    vec3 toCamera = normalize(o_cameraPos - o_position);

    vec3 reflection = normalize(reflect(-env.toLight, normal));
    float shine = pow(max(dot(toCamera, reflection), 0.0), 32);
    vec3 specular = shine * 0.5 * lightColor;

    vec3 reflection2 = normalize(reflect(-toLight2, normal));
    float shine2 = pow(max(dot(toCamera, reflection2), 0.0), 32);
    vec3 specular2 = shine2 * 0.5 * lightColor2;

    vec4 cleanColor = vec4(ambientColor + lightColor  + specular
                                        + lightColor2 + specular2, 1.0)
                    * vec4(objColor, 1.0);

    float dist = distance(o_position, o_cameraPos);
    float farDist = 64.0;
    float ratio = min(dist / farDist, 1.0);
    o_color = (1.0 - ratio) * cleanColor
            + ratio         * vec4(ambientColor, 1.0);
}

