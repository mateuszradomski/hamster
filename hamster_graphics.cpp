struct Mesh
{
	GLuint vao;
	GLuint vbo;
	float *vertices;
};

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