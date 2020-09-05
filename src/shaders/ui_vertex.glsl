#version 330 core
layout (location = 0) in vec2 vertex_pos;

uniform mat4 ortho;
uniform mat4 transform;

out vec2 pixel_texuv;

void main()
{
    vec4 pos = ortho * transform * vec4(vertex_pos, -1.0f, 1.0f);
    pos.z = -1.0f;
    gl_Position = pos;
    pixel_texuv = vec2((vertex_pos.x + 1.0) / 2.0, 1 - (vertex_pos.y + 1.0) / 2.0);
}