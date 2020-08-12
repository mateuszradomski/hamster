#include "hamster_graphics.h"

static OBJModel *
obj_load(const char *filename)
{
	FILE *f = fopen(filename, "r");
	assert(f);

	char *line = NULL;
	size_t line_len = 0;

	OBJModel *model = (OBJModel *)malloc(sizeof(OBJModel));
	memset(model, 0, sizeof(*model));
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
			model->vertices.push(Vec3(x, y, z));
		} else if(strings_match(beginning, "vn")) {
			char *xpart = strtok(NULL, " ");
			char *ypart = strtok(NULL, " ");
			char *zpart = strtok(NULL, " ");
			assert(xpart && ypart && zpart);

			f32 x = atof(xpart);
			f32 y = atof(ypart);
			f32 z = atof(zpart);
			model->normals.push(Vec3(x, y, z));
		} else if(strings_match(beginning, "f")) {
			char *part = strtok(NULL, " ");
			model->faces.push(OBJFace { });
			OBJFace *face = &model->faces[model->faces.length - 1];

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

	printf("vertices: %d\tnormals: %d\tfaces: %d\n", model->vertices.length * 3, model->normals.length * 3, model->faces.length);

	return model;
}

static Mesh*
mesh_create_basic()
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

	Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));

	mesh->vao = vao;
	mesh->vbo = vbo;
	mesh->vertices = (float *)malloc(sizeof(vertices));
	memcpy(mesh->vertices, vertices, sizeof(vertices));

	return mesh;
}

static Mesh*
mesh_create_from_obj(const char *filename)
{
	OBJModel *obj = obj_load(filename);

	unsigned int vertices_count = 0;
	unsigned int faces_count = 0;
	for(unsigned int i = 0; i < obj->faces.length; i++)
	{
		assert(obj->faces[i].vertex_ids.length == obj->faces[i].normal_ids.length);	
		unsigned int face_size = obj->faces[i].vertex_ids.length;
		vertices_count += face_size;
		faces_count += (face_size - 2) * 3;
	}

	Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));

	unsigned int vertices_size = vertices_count * sizeof(float) * 6;
	unsigned int indices_size = faces_count * sizeof(unsigned int);
	mesh->vertices = (float *)malloc(vertices_size);
	mesh->indices = (unsigned int *)malloc(indices_size);

	// TODO: This doesn't have to be this complex!!!
	unsigned int vertex_length = 0;
	unsigned int indices_length = 0;
	for(unsigned int i = 0; i < obj->faces.length; i++)
	{
		unsigned int face_size = obj->faces[i].vertex_ids.length;
		for(unsigned int j = 0; j < face_size; ++j)
		{
			// NOTE: We decrement the array index because obj indexes starting from 1
			void *dest = &mesh->vertices[vertex_length];
			void *src = &obj->vertices[obj->faces[i].vertex_ids[j] - 1];
			unsigned int size = sizeof(float) * 3;
			memcpy(dest, src, size);
			vertex_length += 3;

			dest = &mesh->vertices[vertex_length];
			src = &obj->normals[obj->faces[i].normal_ids[j] - 1];
			size = sizeof(float) * 3;
			memcpy(dest, src, size);
			vertex_length += 3;
		}

		int t = 0;
		for(unsigned int k = 0; k < (face_size - 2) * 3; k++)
		{
			unsigned int face_id = (vertex_length / 6) - face_size;
			mesh->indices[indices_length++] = face_id + ((k - (t / 3)) % face_size);
			t++;
		}
	}

	glGenVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);

	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, mesh->vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &mesh->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, mesh->indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	free(obj);

	return mesh;
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
									"void main() {\n"
									"	gl_Position = proj * view * model * vec4(vertex_pos.x, vertex_pos.y, vertex_pos.z, 1.0);\n"
									"}\0";
	
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader);
	bool vertex_compiled = program_shader_check_error(vertex_shader);
	assert(vertex_compiled);

	const char *fragment_shader_src = "#version 330 core\n"
									"out vec4 pixel_color;\n"
									"void main() {\n"
									"	pixel_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
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