static Spotlight 
spotlight_at_camera(Camera cam)
{
    Spotlight spot = {};
    
    spot.position = cam.position;
    spot.direction = cam.front;
    
    return spot;
}

static DirectLight
create_sun(Vec3 direction, f32 ambient_factor, f32 diffuse_factor, f32 specular_factor)
{
    DirectLight result = {};
    
    result.direction = direction;
    result.ambient_part = Vec3(ambient_factor, ambient_factor, ambient_factor);
    result.diffuse_part = Vec3(diffuse_factor, diffuse_factor, diffuse_factor);
    result.specular_part = Vec3(specular_factor, specular_factor, specular_factor);
    
    return result;
}