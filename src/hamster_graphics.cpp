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
	model->vertices = Array<float>();
	model->indices = Array<unsigned int>();

	unsigned int vertices_size = vertices_count * sizeof(float) * 6;
	unsigned int indices_size = faces_count * sizeof(unsigned int);
	model->vertices.reserve(vertices_size);
	model->indices.reserve(indices_size);

	// TODO: This doesn't have to be this complex!!!
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

	free(obj);

	return model;
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
									"	gl_Position = proj * view * model * vec4(vertex_pos.x, vertex_pos.y, vertex_pos.z, 1.0);\n"
									"}\0";
	
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader);
	bool vertex_compiled = program_shader_check_error(vertex_shader);
	assert(vertex_compiled);

	const char *fragment_shader_src = "#version 330 core\n"
									"struct Light {\n"
									"	vec3 position;\n"
									"	vec3 direction;\n"
									"	float cutoff;\n"
									"	float outer_cutoff;\n"
									"};\n"
									"in vec3 pixel_pos;\n"
									"in vec3 pixel_normal;\n"
									"out vec4 pixel_color;\n"
									"uniform Light light;\n"
									"void main() {\n"
									"	vec3 lightdir = normalize(light.position - pixel_pos);\n"
									"	float diff = max(dot(pixel_normal, lightdir), 0.0);\n"
									"	float theta = dot(lightdir, normalize(-light.direction));\n"
									"	float epsilon = light.cutoff - light.outer_cutoff;\n" // Switched because of how cosine works
									"	float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.2, 1.0);\n"
									"	\n"
									"	float shade = diff * intensity;\n"
									"	pixel_color = vec4(shade, shade, shade, 1.0);\n"
									"}";

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