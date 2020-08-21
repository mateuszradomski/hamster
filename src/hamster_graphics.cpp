#include "hamster_graphics.h"

static OBJMesh
obj_load(const char *filename)
{
	FILE *f = fopen(filename, "r");
	assert(f);
	
	char *line = NULL;
	size_t line_len = 0;
	
	OBJMesh mesh = {};
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
			mesh.vertices.push(Vec3(x, y, z));
		} else if(strings_match(beginning, "vn")) {
			char *xpart = strtok(NULL, " ");
			char *ypart = strtok(NULL, " ");
			char *zpart = strtok(NULL, " ");
			assert(xpart && ypart && zpart);
			
			f32 x = atof(xpart);
			f32 y = atof(ypart);
			f32 z = atof(zpart);
			mesh.normals.push(Vec3(x, y, z));
		} else if(strings_match(beginning, "f")) {
			char *part = strtok(NULL, " ");
			mesh.faces.push(OBJMeshFace { });
			OBJMeshFace *face = &mesh.faces[mesh.faces.length - 1];
			
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
	
	printf("vertices: %d\tnormals: %d\tfaces: %d\n", mesh.vertices.length * 3, mesh.normals.length * 3, mesh.faces.length);
	
	return mesh;
}

static Model
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
	
	Model model = {};
	model.meshes.push(Mesh {});
	Mesh *mesh = model.meshes.data;
	
	mesh->vao = vao;
	mesh->vbo = vbo;
	mesh->vertices.reserve(sizeof(vertices));
	
	mesh->vertices.push(Vertex {});
	Vertex *v = &mesh->vertices[mesh->vertices.length - 1];
	v->position = *((Vec3 *)vertices);
	mesh->vertices.push(Vertex {});
	v = &mesh->vertices[mesh->vertices.length - 1];
	v->position = *((Vec3 *)vertices + 3);
	mesh->vertices.push(Vertex {});
	v = &mesh->vertices[mesh->vertices.length - 1];
	v->position = *((Vec3 *)vertices + 6);
	
	return model;
}

static Model
model_create_debug_floor()
{
	Vertex vertices[] = {
		{ { -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
		{ { -1.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
	};
	
	u32 indices[] = {
		0, 1, 2,
		0, 2, 3
	};
	
	Model model = {};
	model.meshes.push(Mesh {});
	Mesh *mesh = model.meshes.data;
	
	mesh->vertices.reserve(sizeof(vertices));
	mesh->vertices.push_array(vertices, ARRAY_LEN(vertices));
	
	mesh->indices.reserve(sizeof(indices));
	mesh->indices.push_array(indices, ARRAY_LEN(indices));
	
	glGenVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);
	
	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	glGenBuffers(1, &mesh->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	
	u32 vertex_size = 8 * sizeof(f32);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size, nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_size, (void *)(3 * sizeof(f32)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertex_size, (void *)(6 * sizeof(f32)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return model;
}

static Model
model_create_from_obj(const char *filename)
{
	OBJMesh obj = obj_load(filename);
	
	unsigned int vertices_count = 0;
	unsigned int faces_count = 0;
	for(unsigned int i = 0; i < obj.faces.length; i++)
	{
		assert(obj.faces[i].vertex_ids.length == obj.faces[i].normal_ids.length);	
		unsigned int face_size = obj.faces[i].vertex_ids.length;
		vertices_count += face_size;
		faces_count += (face_size - 2) * 3;
	}
	
	Model model = {};
	model.meshes.push(Mesh { });
	Mesh *mesh = &model.meshes[model.meshes.length - 1];
	
	unsigned int vertices_size = vertices_count * sizeof(Vertex);
	unsigned int indices_size = faces_count * sizeof(unsigned int);
	mesh->vertices.reserve(vertices_size);
	mesh->indices.reserve(indices_size);
	
	for(unsigned int i = 0; i < obj.faces.length; i++)
	{
		unsigned int face_size = obj.faces[i].vertex_ids.length;
		for(unsigned int j = 0; j < face_size; ++j)
		{
			// NOTE: We decrement the array index because obj indexes starting from 1
			Vec3 vertex = obj.vertices[obj.faces[i].vertex_ids[j] - 1];
			Vec3 normal = obj.normals[obj.faces[i].normal_ids[j] - 1];
			Vec2 texuv = {};
			
			Vertex v = {};
			v.position = vertex;
			v.normal = normal;
			v.texuv = texuv;
			mesh->vertices.push(v);
		}
		
		for(unsigned int k = 0; k < (face_size - 2) * 3; k++)
		{
			unsigned int face_id = mesh->vertices.length - face_size;
			mesh->indices.push(face_id + ((k - (k / 3)) % face_size));
		}
	}
	
	glGenVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);
	
	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, mesh->vertices.data, GL_STATIC_DRAW);
	
	glGenBuffers(1, &mesh->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, mesh->indices.data, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(float), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	model.flags = (ModelFlags)(model.flags | MODEL_FLAGS_MESH_NORMALS_SHADED);
	model.flags = (ModelFlags)(model.flags & ~MODEL_FLAGS_GOURAUD_SHADED);
	
	return model;
}

// Takes a model and recomputes the normals to be smoothed gouraud style
static void
model_gouraud_shade(Model *model)
{
	// NOTE: we only support one mesh right now
	assert(model->meshes.length == 1);
	Mesh *mesh = &model->meshes[0];
	
	// This array acts like a map for the normals like in this psuedocode:
	//   map<vertex_index, normal_index> normal_map;
	//   auto normal = mesh->normals[normal_map[mesh->faces[k].vertex_ids[h]]];
	Map <Vec3, Vec3> normal_map;
	Array<Vec3> normals;
	
	Array<Vec3> seen;
	for(u32 i = 0; i < mesh->vertices.length; i++)
	{
		Vec3 vertex = mesh->vertices[i].position;
		if(seen.contains(vertex)) { continue; }
		seen.push(vertex);
		
		Vec3 combined_normal = mesh->vertices[i].normal;
		for(u32 j = i + 1; j < mesh->vertices.length; j++)
		{
			if(vertex == mesh->vertices[j].position)
			{
				combined_normal = add(mesh->vertices[j].normal, combined_normal);
			}
			
		}
		normal_map[vertex] = noz(combined_normal);
	}
	
	Array<f32> vertices = {};
	vertices.reserve(mesh->vertices.length * sizeof(Vertex));
	for(unsigned int i = 0; i < mesh->vertices.length; i++)
	{
		// NOTE: We decrement the array index because mesh indexes are starting from 1
		vertices.push_array(mesh->vertices[i].position.m, ARRAY_LEN(mesh->vertices[i].position.m));
		vertices.push_array(normal_map[mesh->vertices[i].position].m, ARRAY_LEN(normal_map[mesh->vertices[i].position].m));
		vertices.push_array(mesh->vertices[i].texuv.m, ARRAY_LEN(mesh->vertices[i].texuv.m));
	}
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.length * sizeof(vertices[0]), vertices.data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	model->flags = (ModelFlags)(model->flags | MODEL_FLAGS_GOURAUD_SHADED);
	model->flags = (ModelFlags)(model->flags & ~MODEL_FLAGS_MESH_NORMALS_SHADED);
}

static void
model_mesh_normals_shade(Model *model)
{
	// NOTE: we only support one mesh right now
	assert(model->meshes.length == 1);
	Mesh *mesh = &model->meshes[0];
	
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->vertices.length * sizeof(mesh->vertices[0]), mesh->vertices.data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	model->flags = (ModelFlags)(model->flags | MODEL_FLAGS_MESH_NORMALS_SHADED);
	model->flags = (ModelFlags)(model->flags & ~MODEL_FLAGS_GOURAUD_SHADED);
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
program_create(const char *vertex_shader_src, const char *fragment_shader_src)
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader);
	bool vertex_compiled = program_shader_check_error(vertex_shader);
	assert(vertex_compiled);
	
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
camera_mouse_moved(Camera *cam, f32 dx, f32 dy)
{
	f32 sensitivity = 0.2f; // TODO: move it outside
	
	cam->yaw += to_radians(dx * sensitivity);
	cam->pitch += to_radians(-dy * sensitivity);
	
	if(cam->pitch > to_radians(89.0f)) {
		cam->pitch = to_radians(89.0f);
	} else if(cam->pitch < to_radians(-89.0f)) {
		cam->pitch = to_radians(-89.0f);
	}
	
	cam->front.x = cosf(cam->pitch) * cosf(cam->yaw);
	cam->front.y = sinf(cam->pitch);
	cam->front.z = cosf(cam->pitch) * sinf(cam->yaw);
	cam->front = noz(cam->front);
	camera_calculate_vectors(cam);
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
