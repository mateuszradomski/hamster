/* date = September 5th 2020 2:14 pm */

#ifndef HAMSTER_SCENE_H
#define HAMSTER_SCENE_H

struct Camera
{
	Vec3 position;
	Vec3 front;
	Vec3 up;
	Vec3 right;
	
	f32 yaw;
	f32 pitch;
};

struct Spotlight
{
    Vec3 position;
    Vec3 direction;
    f32 cutoff;
    f32 outer_cutoff;
    
    Vec3 ambient_part;
    Vec3 diffuse_part;
    Vec3 specular_part;
    
    f32 atten_const;
    f32 atten_linear;
    f32 atten_quad;
};

struct DirectLight
{
    Vec3 direction;
    
    Vec3 ambient_part;
    Vec3 diffuse_part;
    Vec3 specular_part;
};

static Spotlight spotlight_at_camera(Camera cam);
static DirectLight create_sun(Vec3 direction, f32 ambient_factor, f32 diffuse_factor, f32 specular_factor);

#endif //HAMSTER_SCENE_H
