#include "hamster_graphics.h"

static Mesh *
obj_load(const char *filename)
{
	FILE *f = fopen(filename, "r");
	assert(f);
	
	char *line = NULL;
	size_t line_len = 0;
	
	Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
	memset(mesh, 0, sizeof(*mesh));
	while(getline(&line, &line_len, f) != -1)
	{
		// TODO: better rules of skipping a line
		if(line[0] != 'v' && line[0] != 'f') {
			continue;
		}
		
		char *beginning = strtok(line, " ");
		assert(beginning);
		
		if(strings_match(beginning, "v")) {
			char *xpart = strtok(NULL, " ");
			char *ypart = strtok(NULL, " ");
			char *zpart = strtok(NULL, " ");
			assert(xpart && ypart && zpart);
			
			f32 x = atof(xpart);
			f32 y = atof(ypart);
			f32 z = atof(zpart);
			mesh->vertices.push(Vec3(x, y, z));
		} else if(strings_match(beginning, "vn")) {
			char *xpart = strtok(NULL, " ");
			char *ypart = strtok(NULL, " ");
			char *zpart = strtok(NULL, " ");
			assert(xpart && ypart && zpart);
			
			f32 x = atof(xpart);
			f32 y = atof(ypart);
			f32 z = atof(zpart);
			mesh->normals.push(Vec3(x, y, z));
		} else if(strings_match(beginning, "f")) {
			char *part = strtok(NULL, " ");
			mesh->faces.push(MeshFace { });
			MeshFace *face = &mesh->faces[mesh->faces.length - 1];
			
			while(part) {
				char *token = strsep(&part, "/");
				
				if(strlen(token) > 0) { 
					face->vertex_ids.push(atoi(token));
				}
				
				token = strsep(&part, "/");
				if(strlen(token) > 0) { 
					// NOTE: not yet implemented
					assert(false);
				}
				
				token = strsep(&part, "/");
				if(strlen(token) > 0) { 
					face->normal_ids.push(atoi(token));
				}
				
				part = strtok(NULL, " ");
			}
		}
	}
	
	fclose(f);
	
	printf("vertices: %d\tnormals: %d\tfaces: %d\n", mesh->vertices.length * 3, mesh->normals.length * 3, mesh->faces.length);
	
	return mesh;
}

static Model*
model_create_basic()
{
	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f,  0.5f, 0.0f
	};
	
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	Model *model = (Model *)malloc(sizeof(Model));
	model->vertices = Array<float>();
	
	model->vao = vao;
	model->vbo = vbo;
	model->vertices.reserve(sizeof(vertices));
	memcpy(model->vertices.data, vertices, sizeof(vertices));
	
	return model;
}

static Model*
model_create_from_obj(const char *filename)
{
	Mesh *obj = obj_load(filename);
	
	unsigned int vertices_count = 0;
	unsigned int faces_count = 0;
	for(unsigned int i = 0; i < obj->faces.length; i++)
	{
		assert(obj->faces[i].vertex_ids.length == obj->faces[i].normal_ids.length);	
		unsigned int face_size = obj->faces[i].vertex_ids.length;
		vertices_count += face_size;
		faces_count += (face_size - 2) * 3;
	}
	
	Model *model = (Model *)malloc(sizeof(Model));
	memset(model, 0, sizeof(Model));
	model->meshes = Array<Mesh *>();
	model->meshes.push(obj);
	
	model->vertices = Array<float>();
	model->indices = Array<unsigned int>();
	unsigned int vertices_size = vertices_count * sizeof(float) * 6;
	unsigned int indices_size = faces_count * sizeof(unsigned int);
	model->vertices.reserve(vertices_size);
	model->indices.reserve(indices_size);
	
	for(unsigned int i = 0; i < obj->faces.length; i++)
	{
		unsigned int face_size = obj->faces[i].vertex_ids.length;
		for(unsigned int j = 0; j < face_size; ++j)
		{
			// NOTE: We decrement the array index because obj indexes starting from 1
			auto src = &obj->vertices[obj->faces[i].vertex_ids[j] - 1];
			model->vertices.push(src->x);
			model->vertices.push(src->y);
			model->vertices.push(src->z);
			
			src = &obj->normals[obj->faces[i].normal_ids[j] - 1];
			model->vertices.push(src->x);
			model->vertices.push(src->y);
			model->vertices.push(src->z);
		}
		
		int t = 0;
		for(unsigned int k = 0; k < (face_size - 2) * 3; k++)
		{
			unsigned int face_id = (model->vertices.length / 6) - face_size;
			model->indices.push(face_id + ((k - (t / 3)) % face_size));
			t++;
		}
	}
	
	glGenVertexArrays(1, &model->vao);
	glBindVertexArray(model->vao);
	
	glGenBuffers(1, &model->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, model->vertices.data, GL_STATIC_DRAW);
	
	glGenBuffers(1, &model->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, model->indices.data, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	model->state = (ModelState)(model->state | MODEL_STATE_MESH_NORMALS_SHADED);
	model->state = (ModelState)(model->state & ~MODEL_STATE_GOURAUD_SHADED);
	
	return model;
}

// Takes a model and recomputes the normals to be smoothed gouraud style
static void
model_gouraud_shade(Model *model)
{
	// NOTE: we only support one mesh right now
	assert(model->meshes.length == 1);
	Mesh *mesh = model->meshes[0];
	
	// This array acts like a map for the normals like in this psuedocode:
	//   map<vertex_index, normal_index> normal_map;
	//   auto normal = mesh->normals[normal_map[mesh->faces[k].vertex_ids[h]]];
	u32 *normal_map = (u32 *)malloc(sizeof(u32) * mesh->vertices.length);
	Array<Vec3> normals;
	
	u32 faces_count = mesh->faces.length;
	Array<u32> seen;
	for(u32 i = 0; i < faces_count; i++)
	{
		for(u32 j = 0; j < mesh->faces[i].vertex_ids.length; j++)
		{
			u32 vertex_id = mesh->faces[i].vertex_ids[j];
			if(seen.contains(vertex_id)) { continue; }
			seen.push(vertex_id);
			
			Vec3 combined_normal = mesh->normals[mesh->faces[i].normal_ids[j]];
			for(u32 k = i + 1; k < faces_count; k++)
			{
				for(u32 h = 0; h < mesh->faces[k].vertex_ids.length; h++)
				{
					if(vertex_id == mesh->faces[k].vertex_ids[h])
					{
						Vec3 normal = mesh->normals[mesh->faces[k].normal_ids[h] - 1];
						combined_normal = add(combined_normal, normal);
						break;
					}
				}
			}
			
			// Keeping this minus one for consistency below
			normal_map[vertex_id - 1] = normals.length;
			normals.push(noz(combined_normal));
		}
	}
	
	unsigned int vertices_size = model->vertices.length * sizeof(float);
	model->vertices.clear();
	model->vertices.reserve(vertices_size);
	
	for(unsigned int i = 0; i < mesh->faces.length; i++)
	{
		for(unsigned int j = 0; j < mesh->faces[i].vertex_ids.length; j++)
		{
			// NOTE: We decrement the array index because mesh indexes are starting from 1
			auto src = &mesh->vertices[mesh->faces[i].vertex_ids[j] - 1];
			model->vertices.push(src->x);
			model->vertices.push(src->y);
			model->vertices.push(src->z);
			
			src = &normals[normal_map[mesh->faces[i].vertex_ids[j] - 1]];
			model->vertices.push(src->x);
			model->vertices.push(src->y);
			model->vertices.push(src->z);
		}
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_size, model->vertices.data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	model->state = (ModelState)(model->state | MODEL_STATE_GOURAUD_SHADED);
	model->state = (ModelState)(model->state & ~MODEL_STATE_MESH_NORMALS_SHADED);
}

static void
model_mesh_normals_shade(Model *model)
{
	// NOTE: we only support one mesh right now
	assert(model->meshes.length == 1);
	Mesh *mesh = model->meshes[0];
	
	unsigned int vertices_size = model->vertices.length * sizeof(float);
	model->vertices.clear();
	model->vertices.reserve(vertices_size);
	
	for(unsigned int i = 0; i < mesh->faces.length; i++)
	{
		unsigned int face_size = mesh->faces[i].vertex_ids.length;
		for(unsigned int j = 0; j < face_size; ++j)
		{
			// NOTE: We decrement the array index because obj indexes starting from 1
			auto src = &mesh->vertices[mesh->faces[i].vertex_ids[j] - 1];
			model->vertices.push(src->x);
			model->vertices.push(src->y);
			model->vertices.push(src->z);
			
			src = &mesh->normals[mesh->faces[i].normal_ids[j] - 1];
			model->vertices.push(src->x);
			model->vertices.push(src->y);
			model->vertices.push(src->z);
		}
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_size, model->vertices.data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	model->state = (ModelState)(model->state | MODEL_STATE_MESH_NORMALS_SHADED);
	model->state = (ModelState)(model->state & ~MODEL_STATE_GOURAUD_SHADED);
}

// Takes a compiled shader, checks if it produced an error
// returns false if it did, true otherwise.
static bool
program_shader_check_error(GLuint shader)
{
	int  success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
	    glGetShaderInfoLog(shader, 512, NULL, infoLog);
	    printf("%s\n", infoLog);
	    return false;
	}
	
	return true;
}

// Takes a linked program, checks if it produced an error
// returns false if it did, true otherwise.
static bool
program_check_error(GLuint program)
{
	int  success;
	char infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success) {
	    glGetProgramInfoLog(program, 512, NULL, infoLog);
	    printf("%s\n", infoLog);
	    return false;
	}
	
	return true;
}

static GLuint
program_create_basic()
{
	const char *vertex_shader_src = "#version 330 core\n"
		"layout (location = 0) in vec3 vertex_pos;\n"
		"layout (location = 1) in vec3 normal;\n"
		"uniform mat4 proj;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"
		"out vec3 pixel_pos;\n"
		"out vec3 pixel_normal;\n"
		"void main() {\n"
		"	pixel_pos = vec3(model * vec4(vertex_pos, 1.0));\n"
		"	pixel_normal = mat3(transpose(inverse(model))) * normal;\n"
		"	gl_Position = proj * view * model * vec4(vertex_pos.xyz, 1.0);\n"
		"}\0";
	
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader);
	bool vertex_compiled = program_shader_check_error(vertex_shader);
	assert(vertex_compiled);
	
	const char *fragment_shader_src = 
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
	out vec4 pixel_color;
uniform vec3 view_pos;
	uniform SpotLight spotlight;
uniform DirectionalLight direct_light;
	void main() {
vec3 view_dir = normalize(view_pos - pixel_pos);
		vec3 spot_shade = calculate_spotlight(spotlight, pixel_normal, pixel_pos, view_dir);
vec3 direct_shade = calculate_direct_light(direct_light, pixel_normal, view_dir);
vec3 result = spot_shade + direct_shade;
		pixel_color = vec4(result, 1.0);
	})";
	
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
	glCompileShader(fragment_shader);
	bool fragment_compiled = program_shader_check_error(fragment_shader);
	assert(fragment_compiled);
	
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	bool program_ok = program_check_error(program);
	assert(program_ok);
	
	return program;
}

static void
camera_calculate_vectors(Camera *cam)
{
	const Vec3 world_up(0.0f, 1.0f, 0.0f);
	cam->right = noz(cross(cam->front, world_up));
	cam->up = noz(cross(cam->right, cam->front));
}

static void 
opengl_set_uniform(GLuint program, const char *name, f32 val)
{
	GLint location = glGetUniformLocation(program, name);
	glUniform1f(location, val);
	assert(glGetError() == GL_NO_ERROR);
}

static void
opengl_set_uniform(GLuint program, const char *name, Vec3 vec)
{
	GLint location = glGetUniformLocation(program, name);
	glUniform3fv(location, 1, vec.m);
	assert(glGetError() == GL_NO_ERROR);
}

static void
opengl_set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose = GL_FALSE)
{
	GLint location = glGetUniformLocation(program, name);
	glUniformMatrix4fv(location, 1, transpose, mat.a1d);
	assert(glGetError() == GL_NO_ERROR);
}
