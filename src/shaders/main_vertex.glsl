#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texuv;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec3 pixel_pos;
out vec3 pixel_normal;
out vec2 pixel_texuv;

void main()
{
	pixel_pos = vec3(model * vec4(vertex_pos, 1.0));
	pixel_normal = mat3(transpose(inverse(model))) * normal;
	pixel_texuv = texuv;
    
	gl_Position = proj * view * model * vec4(vertex_pos.xyz, 1.0);
}
