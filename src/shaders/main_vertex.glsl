#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texuv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
uniform vec3 view_pos;
uniform vec3 light_pos;

out vec3 pixel_pos;
out vec3 pixel_normal;
out vec2 pixel_texuv;
out vec3 tangent_view_pos;
out vec3 tangent_light_pos;
out mat3 in_tbn;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);
    vec3 B = normalize(normalMatrix * bitangent);
    
    mat3 tbn = transpose(mat3(T, B, N));
    
    vec3 frag_pos = vec3(model * vec4(vertex_pos, 1.0));
    pixel_pos = frag_pos;
    tangent_view_pos = view_pos;
    tangent_light_pos = light_pos;
    
	pixel_normal = mat3(transpose(inverse(model))) * normal;
	pixel_texuv = texuv;
    in_tbn = tbn;
    
	gl_Position = proj * view * model * vec4(vertex_pos.xyz, 1.0);
}
