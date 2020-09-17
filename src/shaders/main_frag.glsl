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

vec4 when_gt(vec4 x, vec4 y) {
    return max(sign(x - y), 0.0);
}

vec3 calculate_spotlight(SpotLight light, vec3 light_position, Material material, vec3 diffuse_map, vec3 specular_map, vec3 normal, vec3 pix_pos, vec3 view_dir)
{
    vec3 lightdir = normalize(light_position - pix_pos);
    float diffuse_mul = max(dot(normal, lightdir), 0.0);
    
    //vec3 reflection = reflect(-lightdir, normal);
    vec3 halfway_dir = normalize(lightdir + view_dir);
    float specular_mul = pow(max(dot(normal, halfway_dir), 0.0f), material.specular_exponent);
    
    float dpix = length(pix_pos - light_position);
    float attenuation = 1.0 / (light.atten_const + light.atten_linear * dpix + light.atten_quad * (dpix * dpix));
    
    vec3 ambient = attenuation * light.ambient_component;
    vec3 diffuse = attenuation * light.diffuse_component;
    vec3 specular = attenuation * light.specular_component;
    
    ambient *= material.ambient_component;
    diffuse *= diffuse_mul * material.diffuse_component * diffuse_map;
    specular *= specular_mul * material.specular_component * specular_map;
    vec3 result = ambient + diffuse + specular;
    
    float theta = dot(lightdir, normalize(-light.direction));
	float epsilon = light.cutoff - light.outer_cutoff; // Switched because of how cosine works
	float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);
    result *= intensity;
    
    return result;
}

vec3 calculate_direct_light(DirectionalLight light, Material material, vec3 diffuse_map, vec3 specular_map, vec3 normal, vec3 view_dir)
{
    vec3 lightdir = normalize(-light.direction);
    float diffuse_mul = max(dot(normal, lightdir), 0.0);
    
    //vec3 reflection = reflect(-lightdir, normal);
    vec3 halfway_dir = normalize(lightdir + view_dir);
    float specular_mul = pow(max(dot(normal, halfway_dir), 0.0f), material.specular_exponent);
    
    vec3 ambient = light.ambient_component * material.ambient_component;
    vec3 diffuse = light.diffuse_component * diffuse_map * (diffuse_mul * material.diffuse_component);
    vec3 specular = light.specular_component * specular_map * (specular_mul * material.specular_component);
    return ambient + diffuse + specular; 
}

vec3 eval_point_light(PointLight light, Material material, vec3 diffmap, vec3 specmap, vec3 normal, vec3 pix_pos, vec3 view_dir)
{
    vec3 lightdir = normalize(light.position - pix_pos);
    float contribution = dot(normal, lightdir);
    float diffuse_mul = max(contribution, 0.0);
    
    float mask = 0.0f;
    if(contribution > 0.0f)
    {
        mask = 1.0f;
    }
    
    //vec3 reflection = reflect(-lightdir, normal);
    vec3 halfway_dir = normalize(lightdir + view_dir);
    float specular_mul = pow(max(dot(normal, halfway_dir), 0.0f), material.specular_exponent) * mask;
    
    float dpix = length(pix_pos - light.position);
    float attenuation = 1.0 / (light.atten_const + light.atten_linear * dpix + light.atten_quad * (dpix * dpix));
    
    vec3 ambient = attenuation * light.ambient_part;
    vec3 diffuse = attenuation * light.diffuse_part;
    vec3 specular = attenuation * light.specular_part;
    
    ambient *= material.ambient_component;
    diffuse *= diffuse_mul * material.diffuse_component * diffmap;
    specular *= specular_mul * material.specular_component * specmap;
    vec3 result = ambient + diffuse + specular;
    
    return result;
}

float eval_shadow(vec4 light_moved_pixel_pos)
{
    vec3 projected_uv = light_moved_pixel_pos.xyz / light_moved_pixel_pos.w;
    projected_uv = projected_uv * 0.5 + 0.5;
    
    float closest = texture(shadow_map, projected_uv.xy).r;
    float current = projected_uv.z;
    
    float bias = 0.005;
    float result = current - bias > closest ? 1.0 : 0.0;
    
    if(projected_uv.z > 1.0)
        return 0.0;
    
    return result;
}

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
        float shadow = eval_shadow(light_moved_pixel_pos);
        vec3 result = spot_shade + ((1.0 - shadow) * direct_shade) + point_shade;
        result = clamp(result, 0.0, 1.0);
        
        pixel_color = vec4(result, 1.0);
    }
}
