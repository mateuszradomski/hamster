#ifndef HAMSTER_GRAPHICS_H

#define VERTICES_PER_CUBE 8
#define INDICES_PER_CUBE 24


struct OBJMeshFace
{
	Array<unsigned int> vertex_ids;
	// NOTE: We want to read in the texture in the future but we are
	// not doing it just now. Idealy the obj model would flag what type
	// of face it read
	// unsigned int texture_id;
	Array<unsigned int> normal_ids;
};

struct OBJMesh
{
	// NOTE: We dont support the w element in geo vertex reading 
	Array<Vec3> vertices;
	Array<Vec3> normals;
	Array<OBJMeshFace> faces;
};

struct BasicShaderProgram
{
	GLuint id;
	GLuint model;
	GLuint view;
	GLuint proj;
	GLuint view_pos;
	GLuint spotlight_position;
	GLuint spotlight_direction;
	GLuint spotlight_cutoff;
	GLuint spotlight_outer_cutoff;
	GLuint spotlight_ambient_component;
	GLuint spotlight_diffuse_component;
	GLuint spotlight_specular_component;
	GLuint spotlight_atten_const;
	GLuint spotlight_atten_linear;
	GLuint spotlight_atten_quad;
	GLuint direct_light_direction;
	GLuint direct_light_ambient_component;
	GLuint direct_light_diffuse_component;
	GLuint direct_light_specular_component;
};

struct Vertex
{
	Vec3 position;
	Vec3 normal;
	Vec2 texuv;
};

struct Mesh
{
	Array<Vertex> vertices;
	Array<u32> indices;
	
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
};

// NOTE: refpoint represents the bottom-back-left point in a box, this
// allows us to always add size to the point, instead of subtracting some
// times and adding other.
struct Hitbox
{
	Vec3 refpoint;
	Vec3 size;
};

enum ModelFlags
{
	MODEL_FLAGS_GOURAUD_SHADED = 0x1,
	MODEL_FLAGS_MESH_NORMALS_SHADED = 0x2,
	MODEL_FLAGS_DRAW_HITBOXES = 0x4,
};

struct Model
{
	Array<Mesh> meshes;
	Array<Hitbox> hitboxes;
	ModelFlags flags;
	
	GLuint texture;
};

struct Line
{
    Vec3 point0;
    Vec3 point1;
};

struct Entity
{
	Vec3 position;
	Vec3 size;
	
	Model *model;
};

struct UIElement
{
	Vec2 position;
	Vec2 size;
	
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLuint texture;
};

struct Camera
{
	Vec3 position;
	Vec3 front;
	Vec3 up;
	Vec3 right;
	
	f32 yaw;
	f32 pitch;
};

static OBJMesh obj_load(const char *filename);
static Model model_create_basic();
static Model model_create_debug_floor();
static Model model_create_from_obj(const char *filename);
static void model_gouraud_shade(Model *model);
static void model_mesh_normals_shade(Model *model);

static Hitbox hitbox_create_from_mesh(Mesh *mesh);

static Line line_from_direction(Vec3 origin, Vec3 direction, f32 line_length);

static void entity_draw(Entity entity, GLuint program);
static void entity_draw_hitbox(Entity entity, GLuint program);

static UIElement ui_element_create(Vec2 position, Vec2 size, const char *texture_filename);
static void ui_element_draw(UIElement element);

static bool ray_intersect_triangle(Vec3 ray_origin, Vec3 ray_direction, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal);
static bool ray_intersect_model(Vec3 ray_origin, Vec3 ray_direction, Model model);
static bool ray_intersect_hitbox(Vec3 ray_origin, Vec3 ray_direction, Hitbox *hbox);
static bool ray_intersect_entity(Vec3 ray_origin, Vec3 ray_direction, Entity *entity);

static GLuint texture_create_from_file(const char *filename);
static GLuint texture_create_solid(f32 r, f32 g, f32 b, f32 a);

static bool program_shader_check_error(GLuint shader);
static bool program_check_error(GLuint program);
static GLuint program_create(const char *vertex_shader_src, const char *fragment_shader_src);
static BasicShaderProgram basic_program_build();

static void camera_calculate_vectors(Camera *cam);
static void camera_mouse_moved(Camera *cam, f32 dx, f32 dy);

static void opengl_set_uniform(GLuint program, const char *name, f32 val);
static void opengl_set_uniform(GLuint program, const char *name, Vec3 vec);
static void opengl_set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose = GL_FALSE);

static Vec3 triangle_normal(Vec3 v0, Vec3 v1, Vec3 v2);

const char *main_vertex_shader_src =
R"(#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texuv;
uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
out vec3 pixel_pos;
out vec3 pixel_normal;
out vec2 pixel_texuv;
void main() {
	pixel_pos = vec3(model * vec4(vertex_pos, 1.0));
	pixel_normal = mat3(transpose(inverse(model))) * normal;
	pixel_texuv = texuv;
	gl_Position = proj * view * model * vec4(vertex_pos.xyz, 1.0);
})";

const char *main_fragment_shader_src = 
R"(#version 330 core
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

vec3 calculate_spotlight(SpotLight light, vec3 normal, vec3 pix_pos, vec3 view_dir)
{
vec3 lightdir = normalize(light.position - pix_pos);
float diffuse_mul = max(dot(normal, lightdir), 0.0);

vec3 reflection = reflect(-lightdir, normal);
float specular_mul = pow(max(dot(view_dir, reflection), 0.0f), 64.0f);

float dpix = length(pix_pos - light.position);
float attenuation = 1.0 / (light.atten_const + light.atten_linear * dpix + light.atten_quad * (dpix * dpix));

vec3 ambient = attenuation * light.ambient_component;
vec3 diffuse = attenuation * diffuse_mul * light.diffuse_component;
vec3 specular = attenuation * specular_mul * light.specular_component;
vec3 result = ambient + diffuse + specular;

float theta = dot(lightdir, normalize(-light.direction));
	float epsilon = light.cutoff - light.outer_cutoff; // Switched because of how cosine works
	float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);
result *= intensity;

return result;
}

vec3 calculate_direct_light(DirectionalLight light, vec3 normal, vec3 view_dir)
{
vec3 lightdir = normalize(-light.direction);
float diffuse_mul = max(dot(normal, lightdir), 0.0);

vec3 reflection = reflect(-lightdir, normal);
float specular_mul = pow(max(dot(view_dir, reflection), 0.0f), 64.0f);
vec3 ambient = light.ambient_component;
vec3 diffuse = diffuse_mul * light.diffuse_component;
vec3 specular = specular_mul * light.specular_component;
return ambient + diffuse + specular; 
}

in vec3 pixel_pos;
in vec3 pixel_normal;
in vec2 pixel_texuv;
out vec4 pixel_color;
uniform vec3 view_pos;
uniform SpotLight spotlight;
uniform DirectionalLight direct_light;
uniform sampler2D tex_sampler;
void main() {
vec3 view_dir = normalize(view_pos - pixel_pos);
	vec3 spot_shade = calculate_spotlight(spotlight, pixel_normal, pixel_pos, view_dir);
vec3 direct_shade = calculate_direct_light(direct_light, pixel_normal, view_dir);
vec3 result = spot_shade + direct_shade;
	pixel_color = texture(tex_sampler, pixel_texuv) * vec4(result, 1.0);
})";

const char *line_fragment_shader_src =
R"(
#version 330 core
out vec4 pixel_color;
void main()
{
pixel_color = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

const char *ui_vertex_src =
R"(
#version 330 core
layout (location = 0) in vec2 vertex_pos;

uniform mat4 transform;

out vec2 pixel_texuv;

void main()
{
gl_Position = transform * vec4(vertex_pos, 0.0f, 1.0f);
pixel_texuv = vec2((vertex_pos.x + 1.0) / 2.0, 1 - (vertex_pos.y + 1.0) / 2.0);
}
)";

const char *ui_fragment_src =
R"(
#version 330 core

in vec2 pixel_texuv;
uniform sampler2D tex_sampler;

out vec4 pixel_color;

void main()
{
vec4 tex_color = texture(tex_sampler, pixel_texuv);
if(tex_color.a < 0.01)
{
discard;
}

pixel_color = tex_color;
}
)";

#define HAMSTER_GRAPHICS_H
#endif
