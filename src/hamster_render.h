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

enum ShaderProgram_Id
{
    ShaderProgram_Basic,
    ShaderProgram_Simple,
    ShaderProgram_Skybox,
    ShaderProgram_UI,
    ShaderProgram_Line,
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

struct Camera
{
	Vec3 position;
	Vec3 front;
	Vec3 up;
	Vec3 right;
	
	f32 yaw;
	f32 pitch;
};


enum RenderHeader
{
    RenderType_RenderEntrySkybox,
	//enum_type_render_entry_rect,
	//enum_type_render_entry_rect_space,
	//enum_type_render_entry_rect_matrix,
	//enum_type_render_entry_point,
};

struct RenderEntrySkybox
{
    RenderHeader header;
    Cubemap cube;
};

#if 0
struct render_entry_rect
{
	render_header header;
	V2 origin;
	V2 dim;
	V4 color;
};

struct render_entry_rect_space
{
	render_header header;
	V2 origin;
	V2 x_axis;
	V2 y_axis;
	loaded_image *texture;
	V4 color;
};

struct render_entry_rect_matrix
{
	render_header header;
	V2 origin;
	Mat2 model;
	loaded_image *texture;
	V4 color;
};

struct render_entry_point
{
	render_header header;
	V2 pos;
	f32 size;
	V4 color;
};
#endif

struct RenderQueue
{
	void *entries;
	u32 size;
	u32 max_size;
	u32 len;
};

struct RenderContext
{
    ShaderProgram programs[ShaderProgram_LastElement];
    Camera cam;
    Mat4 lookat;
    Mat4 proj;
};

static RenderQueue render_create_queue(u32 size = KB(16));
static void render_destory_queue(RenderQueue queue);

#define render_push_entry(queue, type) (type *)_render_push_entry(queue, sizeof(type), RenderType_##type)
static void *_render_push_entry(RenderQueue *queue, u32 struct_size, RenderHeader type);
static void render_push_skybox(RenderQueue *queue, Cubemap skybox);
static void render_flush(RenderQueue *queue, RenderContext *ctx);

static void render_load_programs(RenderContext *ctx);

static bool program_shader_ok(GLuint shader);
static bool program_ok(GLuint program);
static ShaderProgram program_create_from_file(const char *vertex_filename, const char *fragment_filename);

static void camera_calculate_vectors(Camera *cam);
static void camera_mouse_moved(Camera *cam, f32 dx, f32 dy);

static void opengl_set_uniform(GLuint program, const char *name, i32 v);
static void opengl_set_uniform(GLuint program, const char *name, f32 val);
static void opengl_set_uniform(GLuint program, const char *name, Vec3 vec);
static void opengl_set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose = GL_FALSE);

#endif //HAMSTER_RENDER_H
