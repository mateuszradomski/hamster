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

struct PointLight
{
    vec3 position;
    
    vec3 ambient_part;
    vec3 diffuse_part;
    vec3 specular_part;
    
    float atten_const;
    float atten_linear;
    float atten_quad;
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
in vec3 tangent_view_pos;
in vec3 tangent_light_pos;
in mat3 in_tbn;
out vec4 pixel_color;

uniform SpotLight spotlight;
uniform DirectionalLight direct_light;
uniform PointLight point_light;
uniform Material material;
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D shadow_map;
uniform bool show_normal_map;
uniform bool use_mapped_normals;

vec4 when_gt(vec4 x, vec4 y);
vec3 calculate_spotlight(SpotLight light, vec3 light_position, Material material, vec3 diffuse_map, vec3 specular_map, vec3 normal, vec3 pix_pos, vec3 view_dir);
vec3 calculate_direct_light(DirectionalLight light, Material material, vec3 diffuse_map, vec3 specular_map, vec3 normal, vec3 view_dir);
vec3 eval_point_light(PointLight light, Material material, vec3 diffmap, vec3 specmap, vec3 normal, vec3 pix_pos, vec3 view_dir);
float eval_shadow(vec4 light_moved_pixel_pos, sampler2D shadow_map);

void main()
{
    if(show_normal_map)
    {
        vec3 mapped_normal = normalize(texture(normal_map, pixel_texuv).rgb);
        mapped_normal = (mapped_normal * 2.0f) - 1.0f;
        
        mapped_normal = normalize(mapped_normal * in_tbn);
        
        pixel_color = vec4(mapped_normal, 1.0f);
    }
    else
    {
        vec3 view_dir = normalize(tangent_view_pos - pixel_pos);
        vec3 diffuse_map_factor = texture(diffuse_map, pixel_texuv).xyz;
        vec3 specular_map_factor = texture(specular_map, pixel_texuv).xyz;
        
        vec3 _normal;
        if(use_mapped_normals)
        {
            vec3 mapped_normal = normalize(texture(normal_map, pixel_texuv).rgb);
            mapped_normal = (mapped_normal * 2.0f) - 1.0f;
            mapped_normal = normalize(mapped_normal * in_tbn);
            
            _normal = normalize(mapped_normal);
        }
        else
        {
            _normal = normalize(pixel_normal);
        }
        
        vec3 spot_shade = calculate_spotlight(spotlight, tangent_light_pos, material, diffuse_map_factor, specular_map_factor, _normal, pixel_pos, view_dir);
        vec3 direct_shade = calculate_direct_light(direct_light, material, diffuse_map_factor, specular_map_factor, _normal, view_dir);
        vec3 point_shade = eval_point_light(point_light, material, diffuse_map_factor, specular_map_factor, _normal, pixel_pos, view_dir);
        float shadow = eval_shadow(light_moved_pixel_pos, shadow_map);
        vec3 result = spot_shade + ((1.0 - shadow) * direct_shade) + point_shade;
        result = clamp(result, 0.0, 1.0);
        
        pixel_color = vec4(result, 1.0);
    }
}
