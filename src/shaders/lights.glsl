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

float eval_shadow(vec4 light_moved_pixel_pos, sampler2D shadow_map)
{
    vec3 projected_uv = light_moved_pixel_pos.xyz / light_moved_pixel_pos.w;
    projected_uv = projected_uv * 0.5 + 0.5;
    
    float closest = texture(shadow_map, projected_uv.xy).r;
    float current = projected_uv.z;
    bool multisampled_shadow = true;
    int sampling_square_length = 3;
    
    float bias = 0.005;
    float result = 0.0f;
    if(multisampled_shadow)
    {
        float strength = 0.0f;
        vec2 inverse_tex_size = 1.0 / textureSize(shadow_map, 0);
        int sample_start = -(sampling_square_length - 1) / 2;
        int sample_end = -sample_start;
        
        for(int y = sample_start; y <= sample_end; y++)
        {
            for(int x = sample_start; x <= sample_end; x++)
            {
                vec2 sampling_point = projected_uv.xy + vec2(x, y) * inverse_tex_size;
                float sample = texture(shadow_map, sampling_point).r;
                strength += current - bias > sample ? 1.0f : 0.0f;
            }
        }
        
        float sampling_area = (sampling_square_length * sampling_square_length);
        result = strength * (1.0f / sampling_area);
    }
    else
    {
        result = current - bias > closest ? 1.0 : 0.0;
    }
    
    if(projected_uv.z > 1.0)
        return 0.0;
    
    return result;
}
