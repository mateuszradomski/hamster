struct Mesh
{
	GLuint vao;
	GLuint vbo;
	float *vertices;
	unsigned int *indices;
};

struct OBJFace
{
	Array<unsigned int> vertex_ids;
	// NOTE: We want to read in the texture in the future but we are
	// not doing it just now. Idealy the obj model would flag what type
	// of face it read
	// unsigned int texture_id;
	Array<unsigned int> normal_ids;
};

struct OBJModel
{
	// NOTE: We dont support the w element in geo vertex reading 
	Array<float> vertices;
	Array<float> normals;
	Array<OBJFace> faces;
};

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

			// TODO: Make a vec3 struct for this
			model->vertices.push(atof(xpart));
			model->vertices.push(atof(ypart));
			model->vertices.push(atof(zpart));
		} else if(strings_match(beginning, "vn")) {
			char *xpart = strtok(NULL, " ");
			char *ypart = strtok(NULL, " ");
			char *zpart = strtok(NULL, " ");

			assert(xpart && ypart && zpart);
			model->normals.push(atof(xpart));
			model->normals.push(atof(ypart));
			model->normals.push(atof(zpart));
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

	printf("vertices: %d\tnormals: %d\tfaces: %d\n", model->vertices.length, model->normals.length, model->faces.length);

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
	Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
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
									"void main() {\n"
									"	gl_Position = vec4(vertex_pos.x, vertex_pos.y, vertex_pos.z, 1.0);\n"
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