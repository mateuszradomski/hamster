#ifndef HAMSTER_GRAPHICS_H

#define VERTICES_PER_CUBE 8
#define INDICES_PER_CUBE 24

#define OBJ_MAX_MATERIALS 10

#define MAIN_VERTEX_FILENAME "src/shaders/main_vertex.glsl"
#define MAIN_FRAG_FILENAME "src/shaders/main_frag.glsl"
#define LINE_FRAG_FILENAME "src/shaders/line_vertex.glsl"
#define UI_VERTEX_FILENAME "src/shaders/ui_vertex.glsl"
#define UI_FRAG_FILENAME "src/shaders/ui_frag.glsl"
#define SKYBOX_VERTEX_FILENAME "src/shaders/skybox_vertex.glsl"
#define SKYBOX_FRAG_FILENAME "src/shaders/skybox_frag.glsl"

enum OBJParseFlags
{
    OBJ_PARSE_FLAG_EMPTY = 0x0,
    OBJ_PARSE_FLAG_TRIANGULATE = 0x1,
    OBJ_PARSE_FLAG_GEN_TANGENTS = 0x2,
};

// TODO(mateusz): This is ok for model that are loaded from blender, because
// i guess it splits them up so each face is max 4 elements. But anything
// loaded from somewhere else might blow this up if used with triangulation
enum OBJMeshFlags
{
    OBJ_MESH_FLAG_FACE_HAS_VERTEX = 0x1,
    OBJ_MESH_FLAG_FACE_HAS_TEXTURE = 0x2,
    OBJ_MESH_FLAG_FACE_HAS_NORMAL = 0x4,
};

struct OBJVILengths
{
    u32 vertices;
    u32 indices;
};

// NOTE(mateusz): No groups support as of right now.
struct OBJMesh
{
	char name[64];
    char mtl_name[64];
    
    Vec3 *vertexes;
    Vec2 *texture_uvs;
	Vec3 *normals;
    Vec3 *tangents;
    u32 *indices;
    u32 vertices_len;
    u32 indices_len;
    OBJMeshFlags flags;
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
    
    OBJMesh *meshes;
    OBJMaterial *materials;
    
    u32 meshes_len;
    u32 materials_len;
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
	Quat rotate;
    
	Model *model;
};

struct Cubemap
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    
    GLuint texture;
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

static OBJVILengths obj_get_mesh_vilength(char *lines, u32 lines_left);
static OBJModel obj_parse(const char *filename, OBJParseFlags flags = OBJ_PARSE_FLAG_EMPTY);
static void obj_model_destory(OBJModel *model);

static void model_load_obj_materials(Model *model, OBJMaterial *materials, u32 count, const char *working_filename);
static Model model_create_basic();
static Model model_create_debug_floor();
static Model model_create_from_obj(OBJModel *obj);
static void model_gouraud_shade(Model *model);
static void model_mesh_normals_shade(Model *model);

static Hitbox hitbox_create_from_mesh(Mesh *mesh);

static Line line_from_direction(Vec3 origin, Vec3 direction, f32 line_length);

static void entity_draw(Entity entity, BasicShaderProgram program);
static void entity_draw_hitbox(Entity entity, GLuint program);

static Cubemap cubemap_create_skybox();
static void cubemap_draw_skybox(Cubemap skybox);

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
static GLuint program_create_from_file(const char *vertex_filename, const char *fragment_filename);
static BasicShaderProgram basic_program_build();

static void camera_calculate_vectors(Camera *cam);
static void camera_mouse_moved(Camera *cam, f32 dx, f32 dy);

static void opengl_set_uniform(GLuint program, const char *name, f32 val);
static void opengl_set_uniform(GLuint program, const char *name, Vec3 vec);
static void opengl_set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose = GL_FALSE);

static Vec3 triangle_normal(Vec3 v0, Vec3 v1, Vec3 v2);

#define HAMSTER_GRAPHICS_H
#endif
