#version 330 core

out vec4 pixel_color;

in vec2 pixel_texuv;

uniform sampler2D color_texture;

void main()
{
	const float gamma = 2.2;
	const float exposure = 1.0;

	// HDRness
	vec3 hdr_color = texture(color_texture, pixel_texuv).rgb;
	vec3 result = vec3(1.0) - exp(-hdr_color * exposure);

	// Gamma correction
	result = pow(result, vec3(1.0 / gamma));

    pixel_color = vec4(result, 1.0);
}
