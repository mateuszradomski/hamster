#version 330 core
layout (location = 0) in vec3 vertex_pos;

uniform mat4 light_proj_view;
uniform mat4 model;

void main()
{
    gl_Position = light_proj_view * model * vec4(vertex_pos, 1.0);
}