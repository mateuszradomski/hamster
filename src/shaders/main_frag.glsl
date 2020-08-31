#version 330 core

struct SpotLight {
	vec3 position;
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

vec3 calculate_spotlight(SpotLight light, Material material, vec3 diffuse_map, vec3 specular_map, vec3 normal, vec3 pix_pos, vec3 view_dir)
{
    vec3 lightdir = normalize(light.position - pix_pos);
    float diffuse_mul = max(dot(normal, lightdir), 0.0);
    
    vec3 reflection = reflect(-lightdir, normal);
    float specular_mul = pow(max(dot(view_dir, reflection), 0.0f), material.specular_exponent);
    
    float dpix = length(pix_pos - light.position);
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
    
    vec3 reflection = reflect(-lightdir, normal);
    float specular_mul = pow(max(dot(view_dir, reflection), 0.0f), material.specular_exponent);
    vec3 ambient = light.ambient_component * material.ambient_component;
    vec3 diffuse = light.diffuse_component * diffuse_map * (diffuse_mul * material.diffuse_component);
    vec3 specular = light.specular_component * specular_map * (specular_mul * material.specular_component);
    return ambient + diffuse + specular; 
}

in vec3 pixel_pos;
in vec3 pixel_normal;
in vec2 pixel_texuv;
out vec4 pixel_color;

uniform vec3 view_pos;
uniform SpotLight spotlight;
uniform DirectionalLight direct_light;
uniform Material material;
uniform sampler2D tex_sampler;
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;

void main()
{
    vec3 view_dir = normalize(view_pos - pixel_pos);
    vec3 diffuse_map_factor = texture(diffuse_map, pixel_texuv).xyz;
    vec3 specular_map_factor = texture(specular_map, pixel_texuv).xyz;
    vec3 mapped_normal = texture(normal_map, pixel_texuv).rgb * 2.0f - 1.0f;
    vec3 _normal = mapped_normal;
	vec3 spot_shade = calculate_spotlight(spotlight, material, diffuse_map_factor, specular_map_factor, _normal, pixel_pos, view_dir);
    vec3 direct_shade = calculate_direct_light(direct_light, material, diffuse_map_factor, specular_map_factor, _normal, view_dir);
    vec3 result = spot_shade + direct_shade;
	pixel_color = texture(tex_sampler, pixel_texuv) * vec4(result, 1.0);
}
