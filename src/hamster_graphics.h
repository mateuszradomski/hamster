#ifndef HAMSTER_GRAPHICS_H

struct MeshFace
{
	Array<unsigned int> vertex_ids;
	// NOTE: We want to read in the texture in the future but we are
	// not doing it just now. Idealy the obj model would flag what type
	// of face it read
	// unsigned int texture_id;
	Array<unsigned int> normal_ids;
};

struct Mesh
{
	// NOTE: We dont support the w element in geo vertex reading 
	Array<Vec3> vertices;
	Array<Vec3> normals;
	Array<MeshFace> faces;
};

enum ModelFlags
{
	MODEL_FLAGS_GOURAUD_SHADED = 0x1,
	MODEL_FLAGS_MESH_NORMALS_SHADED = 0x2,
};

struct Model
{
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	
	Array<Mesh *> meshes;
	Array<f32> vertices;
	Array<unsigned int> indices;
	
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

static Mesh * obj_load(const char *filename);
static Model model_create_basic();
static Model model_create_debug_floor();
static Model model_create_from_obj(const char *filename);
static void model_gouraud_shade(Model *model);
static void model_mesh_normals_shade(Model *model);

static bool program_shader_check_error(GLuint shader);
static bool program_check_error(GLuint program);
static GLuint program_create_basic();

static void camera_calculate_vectors(Camera *cam);
static void camera_mouse_moved(Camera *cam, f32 dx, f32 dy);

static void opengl_set_uniform(GLuint program, const char *name, f32 val);
static void opengl_set_uniform(GLuint program, const char *name, Vec3 vec);
static void opengl_Set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose = GL_FALSE);

#define HAMSTER_GRAPHICS_H
#endif