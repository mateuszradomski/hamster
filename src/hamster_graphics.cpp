#include "hamster_graphics.h"

static OBJModel
obj_load(const char *filename)
{
	FILE *f = fopen(filename, "r");
	assert(f);
	
	char *line = NULL;
	size_t line_len = 0;
	
	OBJModel model = {};
    char mtllib_filename[64] = {};
    OBJObject *current_object;
	while(getline(&line, &line_len, f) != -1)
	{
		if(string_starts_with(line, "#") || string_starts_with(line, "\n")) { continue; }
        
        u32 parts = string_split(line, ' ');
        char *beginning = line;
		
        if(strings_match(beginning, "o")) {
            OBJObject object = {};
            char *name = string_split_next(line);
            assert(strlen(name) < ARRAY_LEN(object.name));
            
            strcpy(object.name, name);
            string_find_and_replace(object.name, '\n', '\0');
            
            model.objects.push(object);
            current_object = &model.objects[model.objects.length - 1];
        } else {
            assert(current_object);
            
            if(strings_match(beginning, "usemtl")) {
                char *mtl_name = string_split_next(line);
                assert(strlen(mtl_name) < ARRAY_LEN(current_object->mtl_name));
                
                strcpy(current_object->mtl_name, mtl_name);
                string_find_and_replace(current_object->mtl_name, '\n', '\0');
            } else if(strings_match(beginning, "mtllib")) {
                char *filename = string_split_next(line);
                assert(strlen(filename) < ARRAY_LEN(mtllib_filename));
                
                strcpy(mtllib_filename, filename);
                string_find_and_replace(mtllib_filename, '\n', '\0');
            } else if(strings_match(beginning, "vt")) {
                // NOTE(mateusz): We want do it before the merged v and vt
                // so we won't go into it, because texture uv's are Vec2's
                // while normals and vertex_positions are Vec3's.
                assert(parts >= 3);
                char *upart = string_split_next(line);
                char *vpart = string_split_next(upart);
                assert(upart && upart);
                
                f32 u = atof(upart);
                f32 v = atof(vpart);
                
                model.texture_uvs.push(Vec2(u, v));
            } else if(string_starts_with(beginning, "v")) {
                assert(parts >= 4);
                char *xpart = string_split_next(line);
                char *ypart = string_split_next(xpart);
                char *zpart = string_split_next(ypart);
                assert(xpart && ypart && zpart);
                
                f32 x = atof(xpart);
                f32 y = atof(ypart);
                f32 z = atof(zpart);
                
                if(beginning[1] == '\0') {
                    model.vertices.push(Vec3(x, y, z));
                } else if(beginning[1] == 'n') {
                    model.normals.push(Vec3(x, y, z));
                } else {
                    // NOTE(mateusz): Unreachable!
                    assert(false);
                }
            } else if(strings_match(beginning, "f")) {
                current_object->faces.push(OBJMeshFace { });
                OBJMeshFace *face = &current_object->faces[current_object->faces.length - 1];
                
                char *part = string_split_next(line);
                // NOTE(mateusz): Minus one because each line contains also the beginning
                for(u32 i = 0; i < parts - 1; i++)
                {
                    // TODO(mateusz): Using the value of parts we can determine the flag status
                    // for each object, saying what face type it is. I just don't want to do it
                    // in a loop.
                    u32 parts = string_split(part, '/');
                    assert(parts != 0);
                    
                    char *token = part;
                    if(strlen(token) > 0) { 
                        face->vertex_ids.push(atoi(token));
                    }
                    
                    token = string_split_next(token);
                    if(strlen(token) > 0) { 
                        face->texture_ids.push(atoi(token));
                    }
                    
                    token = string_split_next(token);
                    if(strlen(token) > 0) { 
                        face->normal_ids.push(atoi(token));
                    }
                    
                    part = string_split_next(token);
                }
            }
        }
	}
	
	fclose(f);
	
	printf("vertices: %d\tnormals: %d\n", model.vertices.length * 3, model.normals.length * 3);
    
    const char *path = strrchr(filename, '/');
    if(path && path - filename > 0)
    {
        path += 1; // Move over to copy also the last '/'
        memmove(mtllib_filename + (u32)(path - filename), mtllib_filename, strlen(mtllib_filename));
        memmove(mtllib_filename, filename, path - filename);
    }
    
	f = fopen(mtllib_filename, "r");
	assert(f);
    
    OBJMaterial *current_material = NULL;
    while(getline(&line, &line_len, f) != -1)
    {
        if(string_starts_with(line, "#") || string_starts_with(line, "\n")) { continue; }
		
        u32 parts = string_split(line, ' ');
        assert(parts);
        char *beginning = line;
		
        if(strings_match(beginning, "newmtl")) {
            OBJMaterial material = {};
            
            char *name = string_split_next(line);
            assert(strlen(name) < ARRAY_LEN(material.name));
            
            strcpy(material.name, name);
            string_find_and_replace(material.name, '\n', '\0');
            
            model.materials.push(material);
            current_material = &model.materials[model.materials.length - 1];
        } else {
            assert(current_material);
            
            if(strings_match(beginning, "Ns")) {
                char *token = string_split_next(line);
                current_material->specular_exponent = atof(token);
            } else if(string_starts_with(beginning, "K")) {
                assert(parts >= 4);
                char *xpart = string_split_next(line);
                char *ypart = string_split_next(xpart);
                char *zpart = string_split_next(ypart);
                assert(xpart && ypart && zpart);
                
                f32 x = atof(xpart);
                f32 y = atof(ypart);
                f32 z = atof(zpart);
                
                switch(beginning[1])
                {
                    case 'a': {
                        current_material->ambient_component = Vec3(x, y, z);
                        break;
                    }
                    case 'd': {
                        current_material->diffuse_component = Vec3(x, y, z);
                        break;
                    }
                    case 's': {
                        current_material->specular_component = Vec3(x, y, z);
                        break;
                    }
                    case 'e': {
                        current_material->emissive_component = Vec3(x, y, z);
                        break;
                    }
                    default: {
                        assert(false);
                    }
                }
            } else if(strings_match(beginning, "d")) {
                char *token = string_split_next(line);
                current_material->visibility = atof(token);
            } else if(strings_match(beginning, "Ni")) {
                char *token = string_split_next(line);
                current_material->refraction_factor = atof(token);
            } else if(strings_match(beginning, "illum")) {
                char *token = string_split_next(line);
                current_material->illumination_flag = atoi(token);
            } else if(strings_match(beginning, "map_Kd")) {
                char *filename = string_split_next(line);
                assert(strlen(filename) < ARRAY_LEN(current_material->diffuse_map_filename));
                
                strcpy(current_material->diffuse_map_filename, filename);
                string_find_and_replace(current_material->diffuse_map_filename, '\n', '\0');
            } else if(strings_match(beginning, "map_Ks")) {
                char *filename = string_split_next(line);
                assert(strlen(filename) < ARRAY_LEN(current_material->specular_map_filename));
                
                strcpy(current_material->specular_map_filename, filename);
                string_find_and_replace(current_material->specular_map_filename, '\n', '\0');
            } else if(strings_match(beginning, "map_Bump") || strings_match(beginning, "map_bump") ||
                      strings_match(beginning, "bump")) {
                char *filename = string_split_next(line);
                assert(strlen(filename) < ARRAY_LEN(current_material->normal_map_filename));
                
                strcpy(current_material->normal_map_filename, filename);
                string_find_and_replace(current_material->normal_map_filename, '\n', '\0');
            }
        }
    }
    
    fclose(f);
	
	return model;
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
	model.texture = texture_create_from_file("data/wood.png");
	
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
	OBJModel obj = obj_load(filename);
    
    Model model = {};
    OBJObject *object = NULL;
    for(u32 oi = 0; oi < obj.objects.length; oi++)
    {
        object = &obj.objects[oi];
        unsigned int vertices_count = 0;
        unsigned int faces_count = 0;
        for(unsigned int i = 0; i < object->faces.length; i++)
        {
            assert(object->faces[i].vertex_ids.length == object->faces[i].normal_ids.length);	
            unsigned int face_size = object->faces[i].vertex_ids.length;
            vertices_count += face_size;
            faces_count += (face_size - 2) * 3;
        }
        
        model.meshes.push(Mesh { });
        Mesh *mesh = &model.meshes[model.meshes.length - 1];
        
        strcpy(mesh->material_name, object->mtl_name);
        mesh->vertices.reserve(vertices_count);
        mesh->indices.reserve(faces_count);
        
        for(unsigned int i = 0; i < object->faces.length; i++)
        {
            unsigned int face_size = object->faces[i].vertex_ids.length;
            for(unsigned int j = 0; j < face_size; ++j)
            {
                // NOTE: We decrement the array index because obj indexes starting from 1
                Vec3 vertex = obj.vertices[object->faces[i].vertex_ids[j] - 1];
                Vec3 normal = obj.normals[object->faces[i].normal_ids[j] - 1];
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
        
        Hitbox hbox = hitbox_create_from_mesh(mesh);
        model.hitboxes.push(hbox);
        unsigned int vertices_size = vertices_count * sizeof(Vertex);
        unsigned int indices_size = faces_count * sizeof(unsigned int);
        
        glGenVertexArrays(1, &mesh->vao);
        glBindVertexArray(mesh->vao);
        
        glGenBuffers(1, &mesh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices_size, mesh->vertices.data, GL_STATIC_DRAW);
        
        glGenBuffers(1, &mesh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, mesh->indices.data, GL_STATIC_DRAW);
        
        printf("vertices.length = %d\tindices.length = %d\n", mesh->vertices.length, mesh->indices.length);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(float), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
	
    for(u32 i = 0; i < obj.materials.length; i++)
    {
        OBJMaterial *obj_mtl = &obj.materials[i];
        model.materials.push(Material {});
        Material *new_material = &model.materials[model.materials.length - 1];
        
        strcpy(new_material->name, obj_mtl->name);
        new_material->specular_exponent = obj_mtl->specular_exponent;
        new_material->ambient_component = obj_mtl->ambient_component;
        new_material->diffuse_component = obj_mtl->diffuse_component;
        new_material->specular_component = obj_mtl->specular_component;
        
        char texture_filename[64] = {};
        const char *path = strrchr(filename, '/');
        if(path && path - filename > 0)
        {
            path += 1; // Move over to copy also the last '/'
            memcpy(texture_filename, filename, path - filename);
        }
        
        if(!string_empty(obj_mtl->diffuse_map_filename))
        {
            memcpy(texture_filename + (u32)(path - filename), obj_mtl->diffuse_map_filename, strlen(obj_mtl->diffuse_map_filename) + 1);
            new_material->diffuse_map = texture_create_from_file(texture_filename);
            new_material->flags = (MaterialFlags)(new_material->flags | MATERIAL_FLAGS_HAS_DIFFUSE_MAP);
        }
        
        if(!string_empty(obj_mtl->specular_map_filename))
        {
            memcpy(texture_filename + (u32)(path - filename), obj_mtl->specular_map_filename, strlen(obj_mtl->specular_map_filename) + 1);
            new_material->specular_map = texture_create_from_file(texture_filename);
            new_material->flags = (MaterialFlags)(new_material->flags | MATERIAL_FLAGS_HAS_SPECULAR_MAP);
        }
        
        if(!string_empty(obj_mtl->normal_map_filename))
        {
            memcpy(texture_filename + (u32)(path - filename), obj_mtl->normal_map_filename, strlen(obj_mtl->normal_map_filename) + 1);
            new_material->normal_map = texture_create_from_file(texture_filename);
            new_material->flags = (MaterialFlags)(new_material->flags | MATERIAL_FLAGS_HAS_NORMAL_MAP);
        }
    }
    
	model.flags = (ModelFlags)(model.flags | MODEL_FLAGS_MESH_NORMALS_SHADED);
	model.flags = (ModelFlags)(model.flags & ~MODEL_FLAGS_GOURAUD_SHADED);
    
	return model;
}

// Takes a model and recomputes the normals to be smoothed gouraud style
static void
model_gouraud_shade(Model *model)
{
    for(u32 i = 0; i < model->meshes.length; i++)
    {
        Mesh *mesh = &model->meshes[0];
        
        // TODO(mateusz): That's slooooooooooow.
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
    }
	
	model->flags = (ModelFlags)(model->flags | MODEL_FLAGS_GOURAUD_SHADED);
	model->flags = (ModelFlags)(model->flags & ~MODEL_FLAGS_MESH_NORMALS_SHADED);
}

static void
model_mesh_normals_shade(Model *model)
{
    for(u32 i = 0; i < model->meshes.length; i++)
    {
        Mesh *mesh = &model->meshes[0];
        
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->vertices.length * sizeof(mesh->vertices[0]), mesh->vertices.data);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
	
	model->flags = (ModelFlags)(model->flags | MODEL_FLAGS_MESH_NORMALS_SHADED);
	model->flags = (ModelFlags)(model->flags & ~MODEL_FLAGS_GOURAUD_SHADED);
}

static Hitbox
hitbox_create_from_mesh(Mesh *mesh)
{
	Vec3 maxpoint = {};
	Vec3 minpoint = {};
	
	for(u32 i = 0; i < mesh->vertices.length; i++)
    {
		maxpoint.x = MAX(mesh->vertices[i].position.x, maxpoint.x);
		maxpoint.y = MAX(mesh->vertices[i].position.y, maxpoint.y);
		maxpoint.z = MAX(mesh->vertices[i].position.z, maxpoint.z);
		
		minpoint.x = MIN(mesh->vertices[i].position.x, minpoint.x);
		minpoint.y = MIN(mesh->vertices[i].position.y, minpoint.y);
		minpoint.z = MIN(mesh->vertices[i].position.z, minpoint.z);	
    }
	
    Hitbox result = {};
    result.refpoint = minpoint;
    result.size = abs(sub(maxpoint, minpoint));
	
    return result;
}

static Line
line_from_direction(Vec3 origin, Vec3 direction, f32 line_length)
{
    Line result = {};
	
    result.point0 = origin;
    result.point1 = add(origin, scale(direction, line_length));
	
    return result;
}

static void
entity_draw(Entity entity, GLuint program)
{
	Mat4 transform = translate(Mat4(1.0f), entity.position);
	transform = scale(transform, entity.size);
	opengl_set_uniform(program, "model", transform);
	
	Model *model = entity.model;
    for(u32 i = 0; i < model->meshes.length; i++)
    {
        glBindVertexArray(model->meshes[i].vao);
        glBindTexture(GL_TEXTURE_2D, model->texture);
        
        glDrawElements(GL_TRIANGLES, model->meshes[i].indices.length, GL_UNSIGNED_INT, NULL);
        
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

static void 
entity_draw_hitbox(Entity entity, GLuint program)
{
	Model *model = entity.model;
    for(u32 i = 0; i < model->meshes.length; i++)
    {
        assert(model->hitboxes.length == model->meshes.length);
        
        Hitbox *hbox = &model->hitboxes[i];
        Vec3 vertices[VERTICES_PER_CUBE] = {
            hbox->refpoint,
            add(hbox->refpoint, Vec3(hbox->size.x, 0.0f, 0.0f)), // x
            add(hbox->refpoint, Vec3(0.0f, hbox->size.y, 0.0f)), // y
            add(hbox->refpoint, Vec3(0.0f, 0.0f, hbox->size.z)), // z
            add(hbox->refpoint, Vec3(hbox->size.x, hbox->size.y, 0.0f)), // xy
            add(hbox->refpoint, Vec3(0.0f, hbox->size.y, hbox->size.z)), // yz
            add(hbox->refpoint, Vec3(hbox->size.x, 0.0f, hbox->size.z)), // xz
            add(hbox->refpoint, hbox->size),
        };
        
        u32 indicies[INDICES_PER_CUBE] = {
            0, 1, // start to x
            1, 4, // x to xy
            4, 2, // xy to y
            2, 0, // y to start
            
            0, 3, // start to z
            1, 6, // x to xz
            4, 7, // xy to xyz
            2, 5, // y to yz
            
            3, 6, // z to xz
            6, 7, // xz to xyz
            7, 5, // xyz to yz
            5, 3, // yz to 
        };
        
        GLuint hbox_vao, hbox_vbo, hbox_ebo = 0;
        
        glGenVertexArrays(1, &hbox_vao);
        glGenBuffers(1, &hbox_vbo);
        glGenBuffers(1, &hbox_ebo);
        
        glBindVertexArray(hbox_vao);
        glBindBuffer(GL_ARRAY_BUFFER, hbox_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hbox_ebo);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 3 * sizeof(f32), nullptr);
        glEnableVertexAttribArray(0);
        
        Mat4 transform = translate(Mat4(1.0f), entity.position);
        transform = scale(transform, entity.size);
        opengl_set_uniform(program, "model", transform);
        
        glDrawElements(GL_LINES, ARRAY_LEN(indicies), GL_UNSIGNED_INT, NULL);
        
        glDeleteVertexArrays(1, &hbox_vao);
        glDeleteBuffers(1, &hbox_vbo);
        glDeleteBuffers(1, &hbox_ebo);
    }
}

static UIElement
ui_element_create(Vec2 position, Vec2 size, const char *texture_filename)
{
	UIElement element = {};
	element.position = position;
	element.size = size;
	
	// TODO: Each element stores the same infomation in it's own vao and vbo,
	// it would be faster and cheaper if each of them used the same one.
	f32 vertices[] = {
		-1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f,
	};
	
	glGenVertexArrays(1, &element.vao);
	glBindVertexArray(element.vao);
	glGenBuffers(1, &element.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, element.vbo);
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), nullptr);
	glEnableVertexAttribArray(0);
	
	element.program = program_create(ui_vertex_src, ui_fragment_src);
	element.texture = texture_create_from_file(texture_filename);
	
	return element;
}

static void
ui_element_draw(UIElement element)
{
	// NOTE: We could bunch up all of the UI elements together and bind the shader program
	// once. That would be more efficient
	glUseProgram(element.program);
	Mat4 transform = translate(Mat4(1.0f), Vec3(element.position.x, element.position.y, 0.0f));
	transform = scale(transform, Vec3(element.size.x, element.size.y));
	opengl_set_uniform(element.program, "transform", transform);
	
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	
	glBindVertexArray(element.vao);
	glBindTexture(GL_TEXTURE_2D, element.texture);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	
	glUseProgram(0);
	glBindVertexArray(0);
}

// Based on:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
static bool
ray_intersect_triangle(Vec3 ray_origin, Vec3 ray_direction,
					   Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal)
{
	f32 zero_treshold = 0.0001f;
	f32 denom = inner(normal, ray_direction);
	if(denom < zero_treshold && denom > -zero_treshold) { return false; }
	
	f32 dist = -inner(normal, v0);
	f32 ray_dist = -(inner(normal, ray_origin) + dist) / denom;
	if(ray_dist < zero_treshold && ray_dist > -zero_treshold) { return false; }
	
	Vec3 edge0 = sub(v1, v0);
	Vec3 edge1 = sub(v2, v1);
	Vec3 edge2 = sub(v0, v2);
	Vec3 plane_hit = add(ray_origin, scale(ray_direction, ray_dist));
	
	Vec3 c0 = sub(plane_hit, v0); 
	Vec3 c1 = sub(plane_hit, v1);
	Vec3 c2 = sub(plane_hit, v2);
	return (inner(normal, cross(edge0, c0)) > 0.0f && inner(normal, cross(edge1, c1)) > 0.0f &&
			inner(normal, cross(edge2, c2)) > 0.0f);
}

static bool
ray_intersect_model(Vec3 ray_origin, Vec3 ray_direction, Model *model)
{
	for(u32 i = 0; i < model->meshes.length; i++)
	{
		Mesh *mesh = &model->meshes[i];
		
		for(u32 t = 0; t < mesh->indices.length; t += 3)
		{
			Vertex vert0 = mesh->vertices[mesh->indices[t + 0]];
			Vertex vert1 = mesh->vertices[mesh->indices[t + 1]];
			Vertex vert2 = mesh->vertices[mesh->indices[t + 2]];
			assert(vert0.normal == vert1.normal && vert1.normal == vert2.normal);
			
			Vec3 normal = vert0.normal;
			Vec3 v0 = vert0.position;
			Vec3 v1 = vert1.position;
			Vec3 v2 = vert2.position;
			if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal))
			{
				printf("t: %d\n", t);
				return true;
			}
		}
	}
	
	return false;
}

static bool 
ray_intersect_mesh_transformed(Vec3 ray_origin, Vec3 ray_direction, Mesh *mesh, Mat4 transform)
{
    for(u32 t = 0; t < mesh->indices.length; t += 3)
    {
        Vertex vert0 = mesh->vertices[mesh->indices[t + 0]];
        Vertex vert1 = mesh->vertices[mesh->indices[t + 1]];
        Vertex vert2 = mesh->vertices[mesh->indices[t + 2]];
        Vec3 v0 = mul(transform, vert0.position);
        Vec3 v1 = mul(transform, vert1.position);
        Vec3 v2 = mul(transform, vert2.position);
        
        // NOTE(mateusz): I don't know if this is going to be faster,
        // but my guess is that it's going to be, because either the
        // model is smoothed or not, so it will be basicly free.
        Vec3 normal = {};
        if(vert0.normal == vert1.normal && vert1.normal == vert2.normal) {
            normal = vert0.normal;
        } else {
            normal = triangle_normal(v0, v1, v2);
        }
        
        if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal))
        {
            return true;
        }
	}
	
	return false;
}

static bool
ray_intersect_hitbox(Vec3 ray_origin, Vec3 ray_direction, Hitbox *hbox)
{
	// NOTE(mateusz): We are using intersect triangle here because it's already
	// implemented but if it's too slow look into square intersection. But I don't
	// really know if it will work 100% of the time with all of the rotations and stuff.
	
	Vec3 vertices[VERTICES_PER_CUBE] = {
		hbox->refpoint,
		add(hbox->refpoint, Vec3(hbox->size.x, 0.0f, 0.0f)), // x
		add(hbox->refpoint, Vec3(0.0f, hbox->size.y, 0.0f)), // y
		add(hbox->refpoint, Vec3(0.0f, 0.0f, hbox->size.z)), // z
		add(hbox->refpoint, Vec3(hbox->size.x, hbox->size.y, 0.0f)), // xy
		add(hbox->refpoint, Vec3(0.0f, hbox->size.y, hbox->size.z)), // yz
		add(hbox->refpoint, Vec3(hbox->size.x, 0.0f, hbox->size.z)), // xz
		add(hbox->refpoint, hbox->size),
	};
	
	Vec3 v0 = {};
	Vec3 v1 = {};
	Vec3 v2 = {};
	Vec3 normal = {};
	
	// Front face
	v0 = vertices[3];
	v1 = vertices[5];
	v2 = vertices[6];
	// NOTE(mateusz): If slow, on the pararell faces we can just negate the normal
	normal = triangle_normal(v0, v1, v2);
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	v0 = vertices[6];
	v1 = vertices[5];
	v2 = vertices[7];
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	// Back face
	v0 = vertices[0];
	v1 = vertices[1];
	v2 = vertices[2];
	normal = triangle_normal(v0, v1, v2);
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	v0 = vertices[1];
	v1 = vertices[4];
	v2 = vertices[2];
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	// Left face
	v0 = vertices[0];
	v1 = vertices[2];
	v2 = vertices[3];
	normal = triangle_normal(v0, v1, v2);
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	v2 = vertices[3];
	v1 = vertices[2];
	v0 = vertices[5];
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	// Right face
	v0 = vertices[1];
	v1 = vertices[4];
	v2 = vertices[6];
	normal = triangle_normal(v0, v1, v2);
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	v0 = vertices[6];
	v1 = vertices[4];
	v2 = vertices[7];
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	// Top face
	v0 = vertices[2];
	v1 = vertices[5];
	v2 = vertices[4];
	normal = triangle_normal(v0, v1, v2);
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	v0 = vertices[4];
	v1 = vertices[5];
	v2 = vertices[7];
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	// Bottom face
	v0 = vertices[0];
	v1 = vertices[1];
	v2 = vertices[3];
	normal = triangle_normal(v0, v1, v2);
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	v0 = vertices[1];
	v1 = vertices[3];
	v2 = vertices[6];
	if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal)) { return true; }
	
	return false;
}

// NOTE(mateusz): This is in no way a final function, it should really be called
// debug somehow. Real intersections will be working a bit different.
static bool 
ray_intersect_entity(Vec3 ray_origin, Vec3 ray_direction, Entity *entity)
{
    Mat4 transform = create_translate(Mat4(1.0f), entity->position);
	transform = scale(transform, entity->size);
    for(u32 i = 0; i < entity->model->hitboxes.length; i++)
    {
        Hitbox transformed_hbox = {};
        transformed_hbox.refpoint = mul(transform, entity->model->hitboxes[i].refpoint);
        transformed_hbox.size = entity->model->hitboxes[i].size;
        bool hitbox_intersect = ray_intersect_hitbox(ray_origin, ray_direction, &transformed_hbox);
        if(hitbox_intersect)
        {
            bool model_intersect = ray_intersect_mesh_transformed(ray_origin, ray_direction,
                                                                  &entity->model->meshes[i], transform);
            if(model_intersect) { return model_intersect; }
        }
    }
	
	return false;
}

static GLuint
texture_create_from_file(const char *filename)
{
	i32 wimg, himg, channelnr = 0;
	u8 *img_pixels = stbi_load(filename, &wimg, &himg, &channelnr, 0);
	assert(img_pixels);
	
	GLint internal_format = 0;
	GLenum display_format = 0;
	switch(channelnr)
	{
		case 3:
		{
			internal_format = GL_RGB;
			display_format = GL_RGB;
			break;
		}
		case 4:
		{
			internal_format = GL_RGBA;
			display_format = GL_RGBA;
			break;
		}
		default:
		{
			assert(false);
		}
	}
	
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, wimg, himg, 0,
				 display_format, GL_UNSIGNED_BYTE, img_pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	free(img_pixels);
	
	return texture;
}

static GLuint
texture_create_solid(f32 r, f32 g, f32 b, f32 a)
{
	u8 pixel[4] = {
		(u8)clamp((u32)(r * 255.0f + 0.5f), 0, 255),
		(u8)clamp((u32)(g * 255.0f + 0.5f), 0, 255),
		(u8)clamp((u32)(b * 255.0f + 0.5f), 0, 255),
		(u8)clamp((u32)(a * 255.0f + 0.5f), 0, 255),
	};
	
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	return texture;
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

static BasicShaderProgram
basic_program_build()
{
	BasicShaderProgram result = {};
	
	result.id = program_create(main_vertex_shader_src, main_fragment_shader_src);
	result.model = glGetUniformLocation(result.id, "model");
	result.view = glGetUniformLocation(result.id, "view");
	result.proj = glGetUniformLocation(result.id, "proj");
	result.view_pos = glGetUniformLocation(result.id, "view_pos");
	result.spotlight_position = glGetUniformLocation(result.id, "spotlight.position");
	result.spotlight_direction = glGetUniformLocation(result.id, "spotlight.direction");
	result.spotlight_cutoff = glGetUniformLocation(result.id, "spotlight.cutoff");
	result.spotlight_outer_cutoff = glGetUniformLocation(result.id, "spotlight.outer_cutoff");
	result.spotlight_ambient_component = glGetUniformLocation(result.id, "spotlight.ambient_component");
	result.spotlight_diffuse_component = glGetUniformLocation(result.id, "spotlight.diffuse_component");
	result.spotlight_specular_component = glGetUniformLocation(result.id, "spotlight.specular_component");
	result.spotlight_atten_const = glGetUniformLocation(result.id, "spotlight.atten_const");
	result.spotlight_atten_linear = glGetUniformLocation(result.id, "spotlight.atten_linear");
	result.spotlight_atten_quad = glGetUniformLocation(result.id, "spotlight.atten_quad");
	result.direct_light_direction = glGetUniformLocation(result.id, "direct_light.direction");
	result.direct_light_ambient_component = glGetUniformLocation(result.id, "direct_light.ambient_component");
	result.direct_light_diffuse_component = glGetUniformLocation(result.id, "direct_light.diffuse_component");
	result.direct_light_specular_component = glGetUniformLocation(result.id, "direct_light.specular_component");
	
	return result;
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
opengl_set_uniform(GLuint program, const char *name, Mat4 mat, GLboolean transpose)
{
	GLint location = glGetUniformLocation(program, name);
	glUniformMatrix4fv(location, 1, transpose, mat.a1d);
	assert(glGetError() == GL_NO_ERROR);
}

// NOTE(mateusz): This funictons returns normalized normal and takes a counter-clockwise
// specified verticies of a triangle, otherwise it can give a opposite answer.
static Vec3 
triangle_normal(Vec3 v0, Vec3 v1, Vec3 v2)
{
	Vec3 result = {};
	
	result = noz(cross(sub(v1, v0), sub(v2, v0)));
	
	return result;
}