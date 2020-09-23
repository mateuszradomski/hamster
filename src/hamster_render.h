/* date = September 3rd 2020 9:45 pm */

#ifndef HAMSTER_RENDER_H
#define HAMSTER_RENDER_H

#define MAIN_VERTEX_FILENAME "src/shaders/main_vertex.glsl"
#define MAIN_FRAG_FILENAME "src/shaders/main_frag.glsl"
#define SIMPLE_VERTEX_FILENAME "src/shaders/simple_vertex.glsl"
#define SIMPLE_FRAG_FILENAME "src/shaders/simple_frag.glsl"
#define LINE_FRAG_FILENAME "src/shaders/line_vertex.glsl"
#define UI_VERTEX_FILENAME "src/shaders/ui_vertex.glsl"
#define UI_FRAG_FILENAME "src/shaders/ui_frag.glsl"
#define SKYBOX_VERTEX_FILENAME "src/shaders/skybox_vertex.glsl"
#define SKYBOX_FRAG_FILENAME "src/shaders/skybox_frag.glsl"
#define HDR_VERTEX_FILENAME "src/shaders/hdr_vertex.glsl"
#define HDR_FRAG_FILENAME "src/shaders/hdr_frag.glsl"
#define SUN_DEPTH_VERTEX_FILENAME "src/shaders/sun_depth_vertex.glsl"
#define SUN_DEPTH_FRAG_FILENAME "src/shaders/sun_depth_frag.glsl"

enum ShaderProgram_Id
{
    ShaderProgram_Basic,
    ShaderProgram_Simple,
    ShaderProgram_Skybox,
    ShaderProgram_UI,
    ShaderProgram_Line,
    ShaderProgram_HDR,
    ShaderProgram_SunDepth,
    ShaderProgram_LastElement,
};

struct ShaderProgram
{
    GLuint id;
    const char *vertex_filename;
    const char *fragment_filename;
    time_t vertex_stamp;
    time_t fragment_stamp;
};

struct ShaderProgramUniforms
{
    GLuint model;
    GLuint view;
    GLuint proj;
    GLuint transform;
    GLuint ortho;
    GLuint tex_sampler;
    GLuint view_pos;
    GLuint light_pos;
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
    GLuint point_light_position;
    GLuint point_light_ambient_part;
    GLuint point_light_diffuse_part;
    GLuint point_light_specular_part;
    GLuint point_light_atten_const;
    GLuint point_light_atten_linear;
    GLuint point_light_atten_quad;
    GLuint show_normal_map;
    GLuint use_mapped_normals;
    GLuint material_ambient_component;
    GLuint material_diffuse_component;
    GLuint material_specular_component;
    GLuint material_specular_exponent;
    GLuint light_proj_view;
    GLuint shadow_map;
};

enum RenderType
{
    RenderType_RenderEntrySkybox,
    RenderType_RenderEntryLine,
    RenderType_RenderEntryHitbox,
    RenderType_RenderEntryUI,
    RenderType_RenderEntryModelNewest,
    RenderType_RenderEntryModel,
};

struct RenderHeader
{
    RenderType type;
    u32 size;
};

struct RenderEntrySkybox
{
    RenderHeader header;
    Cubemap cube;
};

struct RenderEntryLine
{
    RenderHeader header;
    Line line;
};

struct RenderEntryHitbox
{
    RenderHeader header;
    Vec3 position;
    Vec3 size;
    Quat orientation;
    Hitbox *hbox;
    u32 hbox_len;
};

struct RenderEntryUI
{
    RenderHeader header;
    UIElement element;
};

struct RenderEntryModelNewest
{
    RenderHeader header;
    Vec3 position;
    Vec3 size;
    Quat orientation;
    Model *model;
};

struct RenderEntryModel
{
    RenderHeader header;
    Vec3 position;
    Vec3 size;
    Quat orientation;
    Model *model;
};

struct RenderQueue
{
	void *entries;
	u32 size;
	u32 max_size;
	u32 len;
};

typedef u32 RenderContextFlags;
enum 
{
    RENDER_WINDOW_RESIZED = 0x1,
    RENDER_DRAW_HITBOXES = 0x2,
    RENDER_SHOW_NORMAL_MAP = 0x4,
    RENDER_USE_MAPPED_NORMALS = 0x8,
};

struct RenderContext
{
    ShaderProgram programs[ShaderProgram_LastElement];
    ShaderProgramUniforms program_uniforms[ShaderProgram_LastElement];
    GLuint hdr_fbo;
    GLuint color_buffer;
    GLuint rbo_depth;
    
    GLuint sun_fbo;
    GLuint sun_depth_map;
    u32 shadow_map_height;
    Mat4 light_proj_view;
    
    GLuint white_texture;
    GLuint black_texture;
    
    Spotlight spot;
    DirectLight sun;
    PointLight point_light;
    Camera cam;
    f32 aspect_ratio;
    f32 perspective_far;
    f32 perspective_near;
    
    //bool draw_hitboxes;
    //bool show_normal_map;
    //bool use_mapped_normals;
    RenderContextFlags flags;
    
    Mat4 view;
    Mat4 proj;
    Mat4 ortho;
};

static RenderQueue render_create_queue(u32 size = KB(16));
static void render_destory_queue(RenderQueue *queue);

#define render_push_entry(queue, type) (type *)_render_push_entry(queue, sizeof(type), RenderType_##type)
static void *_render_push_entry(RenderQueue *queue, u32 struct_size, RenderType type);
static void render_push_skybox(RenderQueue *queue, Cubemap skybox);
static void render_push_line(RenderQueue *queue, Line line);
static void render_push_hitbox(RenderQueue *queue, Entity entity);
static void render_push_ui(RenderQueue *queue, UIElement element);
static void render_push_model_newest(RenderQueue *queue, Entity entity);
static void render_push_model(RenderQueue *queue, Entity entity);

static void render_prepass(RenderContext *ctx, i32 window_width, i32 window_height);
static void get_frustum_planes(RenderContext *ctx);
static void render_draw_queue(RenderQueue *queue, RenderContext *ctx);
static void render_end(RenderQueue *queue, RenderContext *ctx, i32 window_width, i32 window_height);

static void render_load_programs(RenderContext *ctx);

static bool program_shader_ok(GLuint shader);
static bool program_ok(GLuint program);
static ShaderProgram program_create_from_file(const char *vertex_filename, const char *fragment_filename);

static void camera_calculate_vectors(Camera *cam);
static void camera_mouse_moved(Camera *cam, f32 dx, f32 dy);


static void opengl_set_uniform(GLuint location, i32 v);
static void opengl_set_uniform(GLuint location, f32 val);
static void opengl_set_uniform(GLuint location, Vec3 vec);
static void opengl_set_uniform(GLuint location, Mat4 mat, GLboolean transpose = GL_FALSE);

static void opengl_set_uniform(GLuint program, const char *name, i32 v);
static void opengl_set_uniform(GLuint program, const char *name, f32 val);
static void opengl_set_uniform(GLuint program, const char *name, Vec3 vec);
static void opengl_set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose = GL_FALSE);

#endif //HAMSTER_RENDER_H
