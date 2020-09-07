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

// NOTE(mateusz): This shader differs from the main shader only in
// not taking a diffuse_map. You could simply just pass a vec3(1.0, 1.0, 1.0)
// and it would work the same, still get to keep the same code.
vec3 calculate_spotlight(SpotLight light, vec3 light_position, Material material, vec3 normal, vec3 pix_pos, vec3 view_dir)
{
    vec3 lightdir = normalize(light_position - pix_pos);
    float diffuse_mul = max(dot(normal, lightdir), 0.0);
    
    vec3 reflection = reflect(-lightdir, normal);
    float specular_mul = pow(max(dot(view_dir, reflection), 0.0f), material.specular_exponent);
    
    float dpix = length(pix_pos - light_position);
    float attenuation = 1.0 / (light.atten_const + light.atten_linear * dpix + light.atten_quad * (dpix * dpix));
    
    vec3 ambient = attenuation * light.ambient_component;
    vec3 diffuse = attenuation * light.diffuse_component;
    vec3 specular = attenuation * light.specular_component;
    
    ambient *= material.ambient_component;
    diffuse *= diffuse_mul * material.diffuse_component;
    specular *= specular_mul * material.specular_component;
    vec3 result = ambient + diffuse + specular;
    
    float theta = dot(lightdir, normalize(-light.direction));
	float epsilon = light.cutoff - light.outer_cutoff; // Switched because of how cosine works
	float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);
    result *= intensity;
    
    return result;
}

vec3 calculate_direct_light(DirectionalLight light, Material material, vec3 normal, vec3 view_dir)
{
    vec3 lightdir = normalize(-light.direction);
    float diffuse_mul = max(dot(normal, lightdir), 0.0);
    
    vec3 reflection = reflect(-lightdir, normal);
    float specular_mul = pow(max(dot(view_dir, reflection), 0.0f), material.specular_exponent);
    vec3 ambient = light.ambient_component * material.ambient_component;
    vec3 diffuse = light.diffuse_component * (diffuse_mul * material.diffuse_component);
    vec3 specular = light.specular_component * (specular_mul * material.specular_component);
    return ambient + diffuse + specular; 
}

in vec3 pixel_pos;
in vec3 pixel_normal;
in vec2 pixel_texuv;
out vec4 pixel_color;

uniform vec3 view_pos;
uniform vec3 light_pos;
uniform SpotLight spotlight;
uniform DirectionalLight direct_light;
uniform Material material;
uniform sampler2D tex_sampler;

void main()
{
    vec2 _uv = pixel_texuv;
    vec3 _normal = normalize(pixel_normal);
    vec3 view_dir = normalize(view_pos - pixel_pos);
    
    vec3 spot_shade = calculate_spotlight(spotlight, light_pos, material,
                                          _normal, pixel_pos, view_dir);
    vec3 direct_shade = calculate_direct_light(direct_light, material, _normal, view_dir);
    vec3 result = spot_shade + direct_shade;
    
    pixel_color = texture(tex_sampler, pixel_texuv) * vec4(result, 1.0);
}
