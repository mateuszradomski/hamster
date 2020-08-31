#version 330 core

in vec3 pixel_texuv;
uniform samplerCube skybox_sampler;

out vec4 pixel_color;

void main()
{
    pixel_color = texture(skybox_sampler, pixel_texuv);
}