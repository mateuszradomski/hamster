#ifndef HAMSTER_GRAPHICS_H

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

enum ModelFlags
{
	MODEL_FLAGS_GOURAUD_SHADED = 0x1,
	MODEL_FLAGS_MESH_NORMALS_SHADED = 0x2,
};

struct Model
{
	Array<Mesh> meshes;
	ModelFlags flags;
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

static OBJMesh obj_load(const char *filename);
static Model model_create_basic();
static Model model_create_debug_floor();
static Model model_create_from_obj(const char *filename);
static void model_gouraud_shade(Model *model);
static void model_mesh_normals_shade(Model *model);

static bool ray_intersect_model(Vec3 ray_origin, Vec3 ray_direction, Model model);

static bool program_shader_check_error(GLuint shader);
static bool program_check_error(GLuint program);
static GLuint program_create(const char *vertex_shader_src, const char *fragment_shader_src);

static void camera_calculate_vectors(Camera *cam);
static void camera_mouse_moved(Camera *cam, f32 dx, f32 dy);

static void opengl_set_uniform(GLuint program, const char *name, f32 val);
static void opengl_set_uniform(GLuint program, const char *name, Vec3 vec);
static void opengl_Set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose = GL_FALSE);

#define HAMSTER_GRAPHICS_H
#endif