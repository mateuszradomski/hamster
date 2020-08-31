#version 330 core
layout (location = 0) in vec3 vertex_pos;

uniform mat4 view;
uniform mat4 proj;

out vec3 pixel_texuv;

void main()
{
    vec4 point = proj * view * vec4(vertex_pos, 1.0f);
    gl_Position = point.xyww;
    pixel_texuv = vertex_pos;
}