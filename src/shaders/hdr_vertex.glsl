#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texuv;

out vec2 pixel_texuv;

void main()
{
    pixel_texuv = texuv;
    gl_Position = vec4(vertex_pos, 1.0);
}