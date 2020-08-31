#version 330 core

in vec2 pixel_texuv;
uniform sampler2D tex_sampler;

out vec4 pixel_color;

void main()
{
    vec4 tex_color = texture(tex_sampler, pixel_texuv);
    if(tex_color.a < 0.01)
    {
        discard;
    }
    
    pixel_color = tex_color;
}