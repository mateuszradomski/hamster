#ifndef HAMSTER_GRAPHICS_H

#define VERTICES_PER_CUBE 8
#define INDICES_PER_CUBE 24

#define OBJ_MAX_MATERIALS 10

typedef u32 OBJParseFlags;
enum
{
    OBJ_PARSE_FLAG_EMPTY = 0x0,
    OBJ_PARSE_FLAG_TRIANGULATE = 0x1,
    OBJ_PARSE_FLAG_GEN_TANGENTS = 0x2,
    OBJ_PARSE_FLAG_GEN_BITANGETS = 0x4,
    OBJ_PARSE_FLAG_FLIP_UVS = 0x8,
};

typedef u32 OBJMeshFlags;
enum
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
    Vec3 *bitangents;
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

struct Vertices
{
	Vec3 *positions;
	Vec2 *texture_uvs;
	Vec3 *normals;
    Vec3 *tangents;
    Vec3 *bitangents;
};

struct Mesh
{
    char material_name[64];
    u32 *indices;
    Vertices vertices;
    u32 indices_len;
    u32 vertices_len;
	
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
};

// TODO(mateusz): Creating a model for a hitbox each frame is expensive,
// do it just once and store the pointer to it somewhere I GUESS. Maybe even
// inline it for each Hitbox, it's just 12 bytes more to store, so it's not that
// bad.

// NOTE: refpoint represents the bottom-back-left point in a box, this
// allows us to always add size to the point, instead of subtracting some
// times and adding other.
struct Hitbox
{
	Vec3 refpoint;
	Vec3 size;
};

typedef u32 ModelFlags;
enum
{
	MODEL_FLAGS_GOURAUD_SHADED = 0x1,
	MODEL_FLAGS_MESH_NORMALS_SHADED = 0x2,
	MODEL_FLAGS_DRAW_HITBOXES = 0x4,
};

typedef u32 MaterialFlags;
enum
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
};

struct Line
{
    Vec3 point0;
    Vec3 point1;
};

struct Line2D
{
    Vec2 point0;
    Vec2 point1;
};

struct Entity
{
	Vec3 position;
	Vec3 size;
	Quat rotate;
    
	Model *model;
};

struct AxisClickResult
{
    Vec3 direction;
    f32 distance;
    bool clicked;
};

typedef u32 EditorClickState;
enum
{
    NOT_YET_CLICKED = 0x0,
    CLICKED_HOLDING = 0x1,
    CLICKED_LET_GO  = 0x2,
};

struct EditorPickedEntity
{
    Entity *entity;
    
    EditorClickState click_state;
    AxisClickResult last_axis;
    f32 xpicked;
    f32 ypicked;
    Vec3 original_position;
    
    Line x_line;
    Line y_line;
    Line z_line;
    
    Hitbox x_line_hbox;
    Hitbox y_line_hbox;
    Hitbox z_line_hbox;
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
	GLuint texture;
};

static OBJVILengths obj_get_mesh_vilength(char *lines, u32 lines_left);
static OBJModel obj_parse(const char *filename, OBJParseFlags flags = OBJ_PARSE_FLAG_EMPTY);
static void obj_model_destory(OBJModel *model);

static void model_load_obj_materials(Model *model, OBJMaterial *materials, u32 count, const char *working_filename);
static Model model_create_basic();
static Model model_create_debug_floor();
static Model model_create_from_obj(OBJModel *obj);
static void model_finalize_mesh(Mesh *mesh);
static void model_destory(Model model);
static void model_gouraud_shade(Model *model);
static void model_mesh_normals_shade(Model *model);

static Hitbox hitbox_create_from_mesh(Mesh *mesh);
static Hitbox hitbox_as_cylinder(Line line, Vec3 r);
static bool hitbox_in_frustum(Hitbox *hbox, Plane *planes, Mat4 transform);

static Vec2 world_point_to_screen(Vec3 world, Mat4 proj, Mat4 view);
static Vec3 ndc_to_ray_direction(Vec2 ndc, Mat4 proj_inversed, Mat4 view_inversed);
static Vec2 screen_to_ndc(f32 xmouse, f32 ymouse, f32 width, f32 height);
static Line line_from_direction(Vec3 origin, Vec3 direction, f32 line_length);

static Cubemap cubemap_create_skybox();

static UIElement ui_element_create(Vec2 position, Vec2 size, const char *texture_filename);

static bool ray_intersect_triangle(Vec3 ray_origin, Vec3 ray_direction, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal);
static bool ray_intersect_model(Vec3 ray_origin, Vec3 ray_direction, Model *model);
static bool ray_intersect_mesh_transformed(Vec3 ray_origin, Vec3 ray_direction, Model *model, Mat4 transform);
static bool ray_intersect_hitbox(Vec3 ray_origin, Vec3 ray_direction, Hitbox *hbox);
static bool ray_intersect_entity(Vec3 ray_origin, Vec3 ray_direction, Entity *entity);

static AxisClickResult closest_click_result(AxisClickResult xaxis, AxisClickResult yaxis, AxisClickResult zaxis);

static Vec3 triangle_normal(Vec3 v0, Vec3 v1, Vec3 v2);

// NOTE(mateusz): This seems to not fit here, maybe rearange the way things are between
// _render and _graphics.
static GLuint texture_create_from_file(const char *filename);
static GLuint texture_create_solid(f32 r, f32 g, f32 b, f32 a);

#define HAMSTER_GRAPHICS_H
#endif
