#version 460 core

const vec2 data[6] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),

    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
};


out V_OUT
{
    vec4 color;
} frag;


layout(location = 0) uniform ivec2 u_winRes;
layout(location = 1) uniform ivec2 u_position;
layout(location = 2) uniform ivec2 u_size;
layout(location = 3) uniform vec4  u_color;

void main()
{
    vec2 coords = data[gl_VertexID];
    vec2 pos    = ((vec2(u_position) + coords * u_size) / u_winRes) * 2.0 - 1;
    pos.y       = pos.y * -1.0;

    frag.color  = u_color;
    gl_Position = vec4(pos, 1.0, 1.0);
}
