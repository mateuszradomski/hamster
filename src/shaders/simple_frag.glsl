#version 330 core

struct SpotLight {
	vec3 direction;
	float cutoff;
	float outer_cutoff;
    
    vec3 ambient_component;
    vec3 diffuse_component;
    vec3 specular_component;
    
    float atten_const;
    float atten_linear;
    float atten_quad;
};

struct DirectionalLight {
    vec3 direction;
    
    vec3 ambient_component;
    vec3 diffuse_component;
    vec3 specular_component;
};

struct Material
{
    vec3 ambient_component;
    vec3 diffuse_component;
    vec3 specular_component;
    float specular_exponent;
};

in vec3 pixel_pos;
in vec4 light_moved_pixel_pos;
in vec3 pixel_normal;
in vec2 pixel_texuv;
out vec4 pixel_color;

uniform vec3 view_pos;
uniform vec3 light_pos;
uniform SpotLight spotlight;
uniform DirectionalLight direct_light;
uniform Material material;
uniform sampler2D tex_sampler;
uniform sampler2D shadow_map;

float eval_shadow(vec4 light_moved_pixel_pos, sampler2D shadow_map);
// NOTE(mateusz): This shader differs from the main shader only in
// not taking a diffuse_map. You could simply just pass a vec3(1.0, 1.0, 1.0)
// and it would work the same, still get to keep the same code.
vec3 calculate_spotlight(SpotLight light, vec3 light_position, Material material, vec3 diffuse_map, vec3 specular_map, vec3 normal, vec3 pix_pos, vec3 view_dir);
vec3 calculate_direct_light(DirectionalLight light, Material material, vec3 diffuse_map, vec3 specular_map, vec3 normal, vec3 view_dir);

void main()
{
    vec2 _uv = pixel_texuv;
    vec3 _normal = normalize(pixel_normal);
    vec3 view_dir = normalize(view_pos - pixel_pos);
    
    vec3 neutral_diffuse = vec3(1.0, 1.0, 1.0);
    vec3 neutral_specular = vec3(1.0, 1.0, 1.0);
    vec3 spot_shade = calculate_spotlight(spotlight, light_pos, material, neutral_diffuse, neutral_specular, _normal, pixel_pos, view_dir);
    vec3 direct_shade = calculate_direct_light(direct_light, material, neutral_diffuse, neutral_specular, _normal, view_dir);
    float shadow = eval_shadow(light_moved_pixel_pos, shadow_map);
    vec3 result = spot_shade + ((1.0 - shadow) * direct_shade);
    
    pixel_color = texture(tex_sampler, pixel_texuv) * vec4(result, 1.0);
}
