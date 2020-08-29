#ifndef HAMSTER_GRAPHICS_H

#define VERTICES_PER_CUBE 8
#define INDICES_PER_CUBE 24


struct OBJMeshFace
{
	Array<unsigned int> vertex_ids;
    Array<unsigned int> texture_ids;
	Array<unsigned int> normal_ids;
};

enum OBJObjectFlags
{
    OBJ_OBJECT_FLAGS_FACE_HAS_VERTEX = 0x1,
    OBJ_OBJECT_FLAGS_FACE_HAS_TEXTURE = 0x2,
    OBJ_OBJECT_FLAGS_FACE_HAS_NORMAL = 0x4,
};

// NOTE(mateusz): No groups support as of right now.
struct OBJObject
{
	char name[64];
    char mtl_name[64];
    
	Array<OBJMeshFace> faces;
    OBJObjectFlags flags;
};

struct OBJMaterial
{
    char name[64];
    f32 visibility;
    f32 specular_exponent;
    f32 refraction_factor;
    u32 illumination_flag;
    Vec3 ambient_component;
    Vec3 diffuse_component;
    Vec3 specular_component;
    Vec3 emissive_component;
    // NOTE(mateusz): Right now only supports these maps, maybe more in the future.
    char diffuse_map_filename[64];
    char specular_map_filename[64];
    char normal_map_filename[64];
};

struct OBJModel
{
	// NOTE: We dont support the w element in geo vertex reading 
    Array<Vec3> vertices;
    Array<Vec2> texture_uvs;
	Array<Vec3> normals;
    Array<OBJObject> objects;
    Array<OBJMaterial> materials;
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
    GLuint material_ambient_component;
    GLuint material_diffuse_component;
    GLuint material_specular_component;
    GLuint material_specular_exponent;
};

struct Vertex
{
	Vec3 position;
	Vec3 normal;
	Vec2 texuv;
};

struct Mesh
{
    char material_name[64];
    u32 *indices;
    Vertex *vertices;
    u32 indices_len;
    u32 vertices_len;
	
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

enum MaterialFlags
{
    MATERIAL_FLAGS_HAS_DIFFUSE_MAP = 0x1,
    MATERIAL_FLAGS_HAS_SPECULAR_MAP = 0x2,
    MATERIAL_FLAGS_HAS_NORMAL_MAP = 0x4,
};

struct Material
{
    char name[64];
    f32 specular_exponent;
    Vec3 ambient_component;
    Vec3 diffuse_component;
    Vec3 specular_component;
    GLuint diffuse_map;
    GLuint specular_map;
    GLuint normal_map;
    MaterialFlags flags;
};

struct Model
{
    Mesh *meshes;
    Hitbox *hitboxes;
    Material *materials;
    u32 meshes_len;
    u32 hitboxes_len;
    u32 materials_len;
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

static OBJModel obj_load(const char *filename);
static Model model_create_basic();
static Model model_create_debug_floor();
static Model model_create_from_obj(const char *filename);
static void model_gouraud_shade(Model *model);
static void model_mesh_normals_shade(Model *model);

static Hitbox hitbox_create_from_mesh(Mesh *mesh);

static Line line_from_direction(Vec3 origin, Vec3 direction, f32 line_length);

static void entity_draw(Entity entity, BasicShaderProgram program);
static void entity_draw_hitbox(Entity entity, GLuint program);

static UIElement ui_element_create(Vec2 position, Vec2 size, const char *texture_filename);
static void ui_element_draw(UIElement element);

static bool ray_intersect_triangle(Vec3 ray_origin, Vec3 ray_direction, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal);
static bool ray_intersect_model(Vec3 ray_origin, Vec3 ray_direction, Model *model);
static bool ray_intersect_mesh_transformed(Vec3 ray_origin, Vec3 ray_direction, Model *model, Mat4 transform);
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
void main() {
vec3 view_dir = normalize(view_pos - pixel_pos);
vec3 diffuse_map_factor = texture(diffuse_map, pixel_texuv).xyz;
vec3 specular_map_factor = texture(specular_map, pixel_texuv).xyz;
	vec3 spot_shade = calculate_spotlight(spotlight, material, diffuse_map_factor, specular_map_factor, pixel_normal, pixel_pos, view_dir);
vec3 direct_shade = calculate_direct_light(direct_light, material, diffuse_map_factor, specular_map_factor, pixel_normal, view_dir);
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

 uniform mat4 ortho;
uniform mat4 transform;

out vec2 pixel_texuv;

void main()
{
gl_Position = ortho * transform * vec4(vertex_pos, -1.0f, 1.0f);
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
