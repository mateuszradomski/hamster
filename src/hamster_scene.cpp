static Spotlight 
spotlight_at_camera(Camera cam)
{
    Spotlight spot = {};
    
    spot.position = cam.position;
    spot.direction = cam.front;
    
    return spot;
}