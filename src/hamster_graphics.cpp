#include "hamster_graphics.h"

static OBJVILengths
obj_get_mesh_vilength(char *lines, u32 lines_left)
{
    OBJVILengths result = {};
    
    char *line = string_split_next(lines);
    for(u32 i = 0; i < lines_left; i++)
    {
        if(line[0] == 'f')
        {
            u32 spaces_count = string_get_char_count(line, ' ');
            result.vertices += spaces_count;
            result.indices += (spaces_count - 2) * 3;
        }
        else if(line[0] == 'o')
        {
            break;
        }
        line = string_split_next(line);
    }
    
    return result;
}

static OBJModel
obj_parse(const char *filename, OBJParseFlags flags)
{
	FILE *f = fopen(filename, "r");
	assert(f);
    (void)flags;
    
    char *contents = read_file_to_string(f);
    fclose(f);
    
	char *line = NULL;
	char *next_line = contents;
    u32 lines_count = string_split(contents, '\n');
    
    OBJModel model = {};
    u32 mesh_count = string_split_count_starting(contents, lines_count, "o");
    model.meshes = (OBJMesh *)malloc(mesh_count * sizeof(OBJMesh));
    
    // NOTE(mateusz): Needs the space, otherwise v will pick up the vt and vn also.
    u32 vertices_count = string_split_count_starting(contents, lines_count, "v ");
    u32 texuvs_count = string_split_count_starting(contents, lines_count, "vt ");
    u32 normals_count = string_split_count_starting(contents, lines_count, "vn ");
    
    Vec3 *vertices = (Vec3 *)malloc(vertices_count * sizeof(Vec3));
    Vec2 *texture_uvs = (Vec2 *)malloc(texuvs_count * sizeof(Vec2));
    Vec3 *normals = (Vec3 *)malloc(normals_count * sizeof(Vec3));
    u32 vertices_len = 0;
    u32 texuvs_len = 0;
    u32 normals_len = 0;
    
    u32 triangles_count = 0;
    char mtllib_filename[64] = {};
    OBJMesh *current_mesh = NULL;
    for(u32 linenr = 0; linenr < lines_count; linenr++)
	{
        line = next_line;
        next_line = string_split_next(line);
        
		if(string_starts_with(line, "#") || string_starts_with(line, "\n") || string_empty(line)) { continue; }
        
        u32 parts = string_split(line, ' ');
        char *beginning = line;
        
        if(strings_match(beginning, "o")) {
            OBJMesh mesh = {};
            char *name = string_split_next(line);
            assert(strlen(name) < ARRAY_LEN(mesh.name));
            
            string_copy_until(mesh.name, name, '\n');
            
            model.meshes[model.meshes_len++] = mesh;
            current_mesh = &model.meshes[model.meshes_len - 1];
            
            OBJVILengths len = obj_get_mesh_vilength(line, lines_count - linenr);
            current_mesh->vertexes = (Vec3 *)malloc(len.vertices * sizeof(Vec3));
            current_mesh->texture_uvs = (Vec2 *)malloc(len.vertices * sizeof(Vec2));
            current_mesh->normals = (Vec3 *)malloc(len.vertices * sizeof(Vec3));
            current_mesh->tangents = (Vec3 *)malloc(len.vertices * sizeof(Vec3));
            current_mesh->indices = (u32 *)malloc(len.indices * sizeof(u32));
        } else if(strings_match(beginning, "mtllib")) {
            char *filename = string_split_next(line);
            assert(strlen(mtllib_filename) == 0);
            assert(strlen(filename) < ARRAY_LEN(mtllib_filename));
            
            string_copy_until(mtllib_filename, filename, '\n');
        } else {
            assert(current_mesh);
            
            if(strings_match(beginning, "usemtl")) {
                char *mtl_name = string_split_next(line);
                assert(strlen(mtl_name) < ARRAY_LEN(current_mesh->mtl_name));
                
                string_copy_until(current_mesh->mtl_name, mtl_name, '\n');
            } else if(strings_match(beginning, "vt")) {
                // NOTE(mateusz): We want do it before the merged v and vt
                // so we won't go into it, because texture uv's are Vec2's
                // while normals and vertex_positions are Vec3's.
                assert(parts >= 3);
                char *upart = string_split_next(line);
                char *vpart = string_split_next(upart);
                assert(upart && upart);
                
                f32 u = string_to_float(upart);
                f32 v = string_to_float(vpart);
                
                texture_uvs[texuvs_len++] = Vec2(u, v);
            } else if(string_starts_with(beginning, "v")) {
                assert(parts == 4);
                char *xpart = string_split_next(line);
                char *ypart = string_split_next(xpart);
                char *zpart = string_split_next(ypart);
                assert(xpart && ypart && zpart);
                
                f32 x = string_to_float(xpart);
                f32 y = string_to_float(ypart);
                f32 z = string_to_float(zpart);
                
                if(beginning[1] == '\0') {
                    vertices[vertices_len++] = Vec3(x, y, z);
                } else if(beginning[1] == 'n') {
                    normals[normals_len++] = Vec3(x, y, z);
                } else {
                    // NOTE(mateusz): Unreachable!
                    assert(false);
                }
            } else if(strings_match(beginning, "f")) {
                char *part = string_split_next(line);
                u32 face_parts = 0;
                u32 face_set = 0;
                
                u32 face_size = parts - 1;
                for(u32 i = 0; i < (face_size - 2) * 3; i++)
                {
                    u32 index = current_mesh->vertices_len + ((i - (i / 3))) % (face_size);
                    current_mesh->indices[current_mesh->indices_len++] = index;
                }
                
                // NOTE(mateusz): Minus one because each line contains also the beginning
                for(u32 i = 0; i < parts - 1; i++)
                {
                    face_set = 0;
                    face_parts = string_split(part, '/');
                    assert(parts != 0);
                    
                    char *token = part;
                    if(strlen(token) > 0) { 
                        face_set++;
                        u32 id = string_to_int(token) - 1;
                        assert(id < vertices_len);
                        current_mesh->vertexes[current_mesh->vertices_len] = vertices[id];
                    }
                    
                    token = string_split_next(token);
                    if(strlen(token) > 0) { 
                        face_set++;
                        u32 id = string_to_int(token) - 1;
                        assert(id < texuvs_len);
                        current_mesh->texture_uvs[current_mesh->vertices_len] = texture_uvs[id];
                    }
                    
                    token = string_split_next(token);
                    if(strlen(token) > 0) { 
                        face_set++;
                        u32 id = string_to_int(token) - 1;
                        assert(id < normals_len);
                        current_mesh->normals[current_mesh->vertices_len] = normals[id];
                    }
                    
                    current_mesh->vertices_len++;
                    part = string_split_next(token);
                }
                
                if(face_parts >= 1) {
                    FLAG_SET(current_mesh->flags, OBJ_MESH_FLAG_FACE_HAS_VERTEX, OBJMeshFlags);
                }
                if(face_parts == 2 || face_set == 3) {
                    FLAG_SET(current_mesh->flags, OBJ_MESH_FLAG_FACE_HAS_TEXTURE, OBJMeshFlags);
                }
                if(face_parts >= 3) {
                    FLAG_SET(current_mesh->flags, OBJ_MESH_FLAG_FACE_HAS_NORMAL, OBJMeshFlags);
                }
                
                // NOTE(mateusz): For minus one see note above, for minus two we are getting the number of triangles for this face.
                triangles_count += parts - 1 - 2;
            }
        }
    }
    
    free(contents);
    
#if 0    
    if(flags & OBJ_PARSE_FLAG_TRIANGULATE)
    {
        for(u32 i = 0; i < model.meshes_len; i++)
        {
            for(u32 j = 0; j < model.meshes[i].faces_len; j++)
            {
                OBJMeshFace *face = &model.meshes[i].faces[j];
                if(face->vids_len > 3)
                {
                    u32 result[ARRAY_LEN(face->vertex_ids)] = {
                        face->vertex_ids[0], face->vertex_ids[1], face->vertex_ids[2]
                    };
                    
                    for(u32 k = 3; k < (face->vids_len - 2) * 3; k++)
                    {
                        result[k] = face->vertex_ids[(k - (k / 3)) % face->vids_len];
                    }
                    
                    memcpy(face->vertex_ids, result, sizeof(result));
                    face->vids_len = (face->vids_len - 2) * 3;
                }
                
                face = &model.meshes[i].faces[j];
                if(face->tids_len > 3)
                {
                    u32 result[ARRAY_LEN(face->texture_ids)] = {
                        face->texture_ids[0], face->texture_ids[1], face->texture_ids[2]
                    };
                    
                    for(u32 k = 3; k < (face->tids_len - 2) * 3; k++)
                    {
                        result[k] = face->texture_ids[(k - (k / 3)) % face->tids_len];
                    }
                    
                    memcpy(face->texture_ids, result, sizeof(result));
                    face->tids_len = (face->tids_len - 2) * 3;
                }
                
                face = &model.meshes[i].faces[j];
                if(face->nids_len > 3)
                {
                    u32 result[ARRAY_LEN(face->normal_ids)] = {
                        face->normal_ids[0], face->normal_ids[1], face->normal_ids[2]
                    };
                    
                    for(u32 k = 3; k < (face->nids_len - 2) * 3; k++)
                    {
                        result[k] = face->normal_ids[(k - (k / 3)) % face->nids_len];
                    }
                    
                    memcpy(face->normal_ids, result, sizeof(result));
                    face->nids_len = (face->nids_len - 2) * 3;
                }
            }
        }
    }
#endif
    
    
#if 0    
    if(flags & OBJ_PARSE_FLAG_GEN_TANGENTS)
    {
        model.tangents = (Vec3 *)malloc(triangles_count * sizeof(Vec3));
        for(u32 i = 0; i < model.meshes_len; i++)
        {
            for(u32 j = 0; j < model.meshes[i].faces_len; j++)
            {
                OBJMeshFace *face = &model.meshes[i].faces[j];
                
                for(u32 k = 0; k < face->vids_len - 2; k++)
                {
                    u32 v0_id = face->normal_ids[((k + 0) - ((k + 0) / 3)) % face->nids_len];
                    u32 v1_id = face->normal_ids[((k + 1) - ((k + 1) / 3)) % face->nids_len];
                    u32 v2_id = face->normal_ids[((k + 2) - ((k + 2) / 3)) % face->nids_len];
                    
                    Vec3 pos_v0 = model.vertices[v0_id - 1];
                    Vec3 pos_v1 = model.vertices[v1_id - 1];
                    Vec3 pos_v2 = model.vertices[v2_id - 1];
                    
                    Vec2 uv_v0 = model.texture_uvs[v0_id - 1];
                    Vec2 uv_v1 = model.texture_uvs[v1_id - 1];
                    Vec2 uv_v2 = model.texture_uvs[v2_id - 1];
                    
                    Vec3 delta_pos0 = sub(pos_v1, pos_v0);
                    Vec3 delta_pos1 = sub(pos_v2, pos_v0);
                    Vec2 delta_uv0  = sub(uv_v1, uv_v0);
                    Vec2 delta_uv1  = sub(uv_v2, uv_v0);
                    
                    f32 rdet = 1.0f / (delta_uv0.x * delta_uv1.y - delta_uv0.y * delta_uv1.x);
                    Vec3 tangent = sub(scale(delta_pos0, delta_uv1.y),
                                       scale(delta_pos1, -delta_uv0.y));
                    tangent = scale(tangent, rdet);
                    assert(model.tangents_len < triangles_count);
                    (void)tangent;
                    model.meshes[i].triangles[j].tangent = tangent;
                    model.tangets[model.tangets_len++] = tangent;
                    // TODO(mateusz): Calculate Bitangents?
                }
            }
        }
    }
#endif
    
    const char *path = strrchr(filename, '/');
    if(path && path - filename > 0)
    {
        path += 1; // Move over to copy also the last '/'
        memmove(mtllib_filename + (u32)(path - filename), mtllib_filename, strlen(mtllib_filename));
        memmove(mtllib_filename, filename, path - filename);
    }
    
    f = fopen(mtllib_filename, "r");
    assert(f);
    
    contents = read_file_to_string(f);
    fclose(f);
    
    line = NULL;
    next_line = contents;
    lines_count = string_split(contents, '\n');
    
    u32 materials_count = string_split_count_starting(contents, lines_count, "newmtl");
    model.materials = (OBJMaterial *)malloc(materials_count * sizeof(OBJMaterial));
    
    OBJMaterial *current_material = NULL;
    for(u32 linenr = 0; linenr < lines_count; linenr++)
    {
        line = next_line;
        next_line = string_split_next(line);
        if(string_starts_with(line, "#") || string_starts_with(line, "\n") || string_empty(line)) { continue; }
        
        u32 parts = string_split(line, ' ');
        assert(parts);
        char *beginning = line;
        
        if(strings_match(beginning, "newmtl")) {
            OBJMaterial material = {};
            
            char *name = string_split_next(line);
            assert(strlen(name) < ARRAY_LEN(material.name));
            
            string_copy_until(material.name, name, '\n');
            
            model.materials[model.materials_len++] = material;
            current_material = &model.materials[model.materials_len - 1];
        } else {
            assert(current_material);
            
            if(strings_match(beginning, "Ns")) {
                char *token = string_split_next(line);
                current_material->specular_exponent = string_to_float(token);
            } else if(string_starts_with(beginning, "K")) {
                assert(parts >= 4);
                char *xpart = string_split_next(line);
                char *ypart = string_split_next(xpart);
                char *zpart = string_split_next(ypart);
                assert(xpart && ypart && zpart);
                
                f32 x = string_to_float(xpart);
                f32 y = string_to_float(ypart);
                f32 z = string_to_float(zpart);
                
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
                current_material->visibility = string_to_float(token);
            } else if(strings_match(beginning, "Ni")) {
                char *token = string_split_next(line);
                current_material->refraction_factor = string_to_float(token);
            } else if(strings_match(beginning, "illum")) {
                char *token = string_split_next(line);
                current_material->illumination_flag = string_to_int(token);
            } else if(strings_match(beginning, "map_Kd")) {
                char *filename = string_split_next(line);
                assert(strlen(filename) < ARRAY_LEN(current_material->diffuse_map_filename));
                
                string_copy_until(current_material->diffuse_map_filename, filename, '\n');
            } else if(strings_match(beginning, "map_Ks")) {
                char *filename = string_split_next(line);
                assert(strlen(filename) < ARRAY_LEN(current_material->specular_map_filename));
                
                string_copy_until(current_material->specular_map_filename, filename, '\n');
            } else if(strings_match(beginning, "map_Bump") || strings_match(beginning, "map_bump") ||
                      strings_match(beginning, "bump")) {
                char *filename = string_split_next(line);
                assert(strlen(filename) < ARRAY_LEN(current_material->normal_map_filename));
                
                string_copy_until(current_material->normal_map_filename, filename, '\n');
            }
        }
    }
    
    free(contents);
    
    return model;
}

static void
obj_model_destory(OBJModel *model)
{
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        free(model->meshes[i].vertexes);
        free(model->meshes[i].texture_uvs);
        free(model->meshes[i].normals);
        free(model->meshes[i].tangents);
    }
    free(model->meshes);
    free(model->materials);
}

static void 
model_load_obj_materials(Model *model, OBJMaterial *materials, u32 count, const char *working_filename)
{
    model->materials = (Material *)malloc(count * sizeof(Material));
    for(u32 i = 0; i < count; i++)
    {
        OBJMaterial *obj_mtl = &materials[i];
        model->materials[model->materials_len++] = {};
        Material *new_material = &model->materials[model->materials_len - 1];
        
        strcpy(new_material->name, obj_mtl->name);
        new_material->specular_exponent = obj_mtl->specular_exponent;
        new_material->ambient_component = obj_mtl->ambient_component;
        new_material->diffuse_component = obj_mtl->diffuse_component;
        new_material->specular_component = obj_mtl->specular_component;
        
        char texture_filename[64] = {};
        const char *path = strrchr(working_filename, '/');
        i32 path_length = (i32)(path - working_filename);
        if(path && path_length > 0)
        {
            path += 1; // Move over to copy also the last '/'
            path_length += 1;
            memcpy(texture_filename, working_filename, path_length);
        }
        
        if(!string_empty(obj_mtl->diffuse_map_filename))
        {
            strcpy(texture_filename + path_length, obj_mtl->diffuse_map_filename);
            new_material->diffuse_map = texture_create_from_file(texture_filename);
            
            glBindTexture(GL_TEXTURE_2D, new_material->diffuse_map);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            FLAG_SET(new_material->flags, MATERIAL_FLAGS_HAS_DIFFUSE_MAP, MaterialFlags);
        }
        
        if(!string_empty(obj_mtl->specular_map_filename))
        {
            strcpy(texture_filename + path_length, obj_mtl->specular_map_filename);
            new_material->specular_map = texture_create_from_file(texture_filename);
            
            glBindTexture(GL_TEXTURE_2D, new_material->specular_map);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            FLAG_SET(new_material->flags, MATERIAL_FLAGS_HAS_SPECULAR_MAP, MaterialFlags);
        }
        
        if(!string_empty(obj_mtl->normal_map_filename))
        {
            strcpy(texture_filename + path_length, obj_mtl->normal_map_filename);
            new_material->normal_map = texture_create_from_file(texture_filename);
            
            glBindTexture(GL_TEXTURE_2D, new_material->specular_map);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            FLAG_SET(new_material->flags, MATERIAL_FLAGS_HAS_NORMAL_MAP, MaterialFlags);
        }
    }
}

#if 0
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
    model.meshes = (Mesh *)malloc(sizeof(Mesh));
    model.meshes[model.meshes_len++] = {};
    Mesh *mesh = model.meshes;
    
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
#endif

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
    
    model.meshes = (Mesh *)malloc(sizeof(Mesh));
    model.meshes[model.meshes_len++] = {};
    Mesh *mesh = model.meshes;
    
    mesh->vertices = (Vertex *)malloc(sizeof(vertices));
    for(u32 i = 0; i < ARRAY_LEN(vertices); i++)
    {
        mesh->vertices[mesh->vertices_len] = vertices[i];
    }
    
    mesh->indices = (u32 *)malloc(sizeof(indices));
    for(u32 i = 0; i < ARRAY_LEN(indices); i++)
    {
        mesh->indices[mesh->indices_len++] = indices[i];
    }
    
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
model_create_from_obj(OBJModel *obj)
{
    Model model = {};
    
    OBJMesh *objmesh = NULL;
    model.meshes = (Mesh *)malloc(obj->meshes_len * sizeof(Mesh));
    for(u32 oi = 0; oi < obj->meshes_len; oi++)
    {
        objmesh = &obj->meshes[oi];
        unsigned int vertices_count = objmesh->vertices_len;
        unsigned int faces_count = objmesh->indices_len;
        
        model.meshes[model.meshes_len++] = {};
        Mesh *mesh = &model.meshes[model.meshes_len - 1];
        
        strcpy(mesh->material_name, objmesh->mtl_name);
        mesh->vertices = (Vertex *)malloc(vertices_count * sizeof(Vertex));
        for(unsigned int j = 0; j < objmesh->vertices_len; ++j)
        {
            // NOTE: We decrement the array index because obj indexes starting from 1
            Vec3 vertex = (objmesh->flags & OBJ_MESH_FLAG_FACE_HAS_VERTEX) ? objmesh->vertexes[j] : Vec3(0.0f, 0.0f, 0.0f);
            Vec3 normal = (objmesh->flags & OBJ_MESH_FLAG_FACE_HAS_NORMAL) ? objmesh->normals[j] : Vec3(0.0f, 0.0f, 0.0f);
            Vec2 texuv = (objmesh->flags & OBJ_MESH_FLAG_FACE_HAS_TEXTURE) ? objmesh->texture_uvs[j] : Vec2(0.0f, 0.0f);
            
            
            Vertex *v = &mesh->vertices[mesh->vertices_len++];
            v->position = vertex;
            v->normal = normal;
            v->texuv = texuv;
        }
        
        mesh->indices = (u32 *)malloc(faces_count * sizeof(u32));
        for(u32 j = 0; j < objmesh->indices_len; j++)
        {
            mesh->indices[mesh->indices_len++] = objmesh->indices[j];
        }
        
        model.hitboxes = (Hitbox *)malloc(obj->meshes_len * sizeof(Hitbox));
        model.hitboxes[model.hitboxes_len++] = hitbox_create_from_mesh(mesh);
        unsigned int vertices_size = vertices_count * sizeof(Vertex);
        unsigned int indices_size = faces_count * sizeof(unsigned int);
        
        glGenVertexArrays(1, &mesh->vao);
        glBindVertexArray(mesh->vao);
        
        glGenBuffers(1, &mesh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices_size, mesh->vertices, GL_STATIC_DRAW);
        
        glGenBuffers(1, &mesh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, mesh->indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    FLAG_SET(model.flags, MODEL_FLAGS_MESH_NORMALS_SHADED, ModelFlags);
    FLAG_UNSET(model.flags, MODEL_FLAGS_GOURAUD_SHADED, ModelFlags);
    
    return model;
}

// Takes a model and recomputes the normals to be smoothed gouraud style
static void
model_gouraud_shade(Model *model)
{
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        Mesh *mesh = &model->meshes[0];
        
        // TODO(mateusz): That's slooooooooooow.
        Map <Vec3, Vec3> normal_map;
        
        u32 seen_len = 0;
        Vec3 *seen = (Vec3 *)malloc(mesh->vertices_len * sizeof(Vec3));
        for(u32 i = 0; i < mesh->vertices_len; i++)
        {
            bool seen_contains = false;
            Vec3 vertex = mesh->vertices[i].position;
            for(u32 j = 0; j < seen_len; j++)
            {
                if(seen[j] == vertex)
                { 
                    seen_contains = true;
                    break;
                }
            }
            if(seen_contains) { continue; }
            seen[seen_len++] = vertex;
            
            Vec3 combined_normal = mesh->vertices[i].normal;
            for(u32 j = i + 1; j < mesh->vertices_len; j++)
            {
                if(vertex == mesh->vertices[j].position)
                {
                    combined_normal = add(mesh->vertices[j].normal, combined_normal);
                }
                
            }
            normal_map[vertex] = noz(combined_normal);
        }
        
        free(seen);
        
        u32 vertices_len = 0;
        Vertex *vertices = (Vertex *)malloc(mesh->vertices_len * sizeof(Vertex));
        for(unsigned int i = 0; i < mesh->vertices_len; i++)
        {
            // NOTE: We decrement the array index because mesh indexes are starting from 1
            vertices[vertices_len].position = mesh->vertices[i].position;
            vertices[vertices_len].normal = normal_map[mesh->vertices[i].position];
            vertices[vertices_len].texuv = mesh->vertices[i].texuv;
            vertices_len++;
        }
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_len * sizeof(vertices[0]), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        free(vertices);
    }
    
    model->flags = (ModelFlags)(model->flags | MODEL_FLAGS_GOURAUD_SHADED);
    model->flags = (ModelFlags)(model->flags & ~MODEL_FLAGS_MESH_NORMALS_SHADED);
}

static void
model_mesh_normals_shade(Model *model)
{
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        Mesh *mesh = &model->meshes[0];
        
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->vertices_len * sizeof(mesh->vertices[0]), mesh->vertices);
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
    
    for(u32 i = 0; i < mesh->vertices_len; i++)
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
entity_draw(Entity entity, BasicShaderProgram program)
{
    Mat4 transform = scale(Mat4(1.0f), entity.size);
    transform = rotate_quat(transform, entity.rotate);
    transform = translate(transform, entity.position);
    glUniformMatrix4fv(program.model, 1, GL_FALSE, transform.a1d);
    
    Model *model = entity.model;
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        glBindVertexArray(model->meshes[i].vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->texture);
        
        Mesh *mesh = &model->meshes[i];
        Material *material = NULL;
        for(u32 i = 0; i < model->materials_len; i++)
        {
            if(strings_match(mesh->material_name, model->materials[i].name))
            {
                material = &model->materials[i];
                break;
            }
        }
        
        if(material)
        {
            if(material->flags & MATERIAL_FLAGS_HAS_DIFFUSE_MAP)
            {
                glUniform1i(glGetUniformLocation(program.id, "diffuse_map"), 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, material->diffuse_map);
            }
            if(material->flags & MATERIAL_FLAGS_HAS_SPECULAR_MAP)
            {
                glUniform1i(glGetUniformLocation(program.id, "specular_map"), 2);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, material->specular_map);
            }
            if(material->flags & MATERIAL_FLAGS_HAS_NORMAL_MAP)
            {
                glUniform1i(glGetUniformLocation(program.id, "normal_map"), 3);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, material->normal_map);
            }
            
            glUniform3fv(program.material_ambient_component, 1, material->ambient_component.m);
            glUniform3fv(program.material_diffuse_component, 1, material->diffuse_component.m);
            glUniform3fv(program.material_specular_component, 1, material->specular_component.m);
            glUniform1f(program.material_specular_exponent, material->specular_exponent);
        }
        else
        {
            Vec3 one_vec = Vec3(1.0f, 1.0f, 1.0f);
            glUniform3fv(program.material_ambient_component, 1, one_vec.m);
            glUniform3fv(program.material_diffuse_component, 1, one_vec.m);
            glUniform3fv(program.material_specular_component, 1, one_vec.m);
            glUniform1f(program.material_specular_exponent, 1.0f);
        }
        
        glDrawElements(GL_TRIANGLES, mesh->indices_len, GL_UNSIGNED_INT, NULL);
        glUniform1i(glGetUniformLocation(program.id, "diffuse_map"), 0);
        glUniform1i(glGetUniformLocation(program.id, "specular_map"), 0);
        glUniform1i(glGetUniformLocation(program.id, "normal_map"), 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

static void 
entity_draw_hitbox(Entity entity, GLuint program)
{
    Model *model = entity.model;
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        assert(model->hitboxes_len == model->meshes_len);
        
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr);
        glEnableVertexAttribArray(0);
        
        Mat4 transform = scale(Mat4(1.0f), entity.size);
        transform = rotate_quat(transform, entity.rotate);
        transform = translate(transform, entity.position);
        opengl_set_uniform(program, "model", transform);
        
        glDrawElements(GL_LINES, ARRAY_LEN(indicies), GL_UNSIGNED_INT, NULL);
        
        glDeleteVertexArrays(1, &hbox_vao);
        glDeleteBuffers(1, &hbox_vbo);
        glDeleteBuffers(1, &hbox_ebo);
    }
}

static Cubemap
cubemap_create_skybox()
{
    Cubemap map = {};
    
    const char *filenames[] = {
        "data/skybox/right.jpg",
        "data/skybox/left.jpg",
        "data/skybox/top.jpg",
        "data/skybox/bottom.jpg",
        "data/skybox/front.jpg",
        "data/skybox/back.jpg"
    };
    
    glGenTextures(1, &map.texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, map.texture);
    
    for(u32 i = 0; i < ARRAY_LEN(filenames); i++)
    {
        i32 width, height, channels;
        u8 *pixels = stbi_load(filenames[i], &width, &height, &channels, 0);
        assert(pixels);
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                     width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        free(pixels);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    f32 vertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        
        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
    };
    
    u32 indices[] = {
        0, 1, 2,
        2, 3, 0,
        
        4, 5, 6,
        6, 7, 4,
        
        8, 9, 10,
        10, 11, 8,
        
        12, 13, 14,
        14, 15, 12,
        
        16, 17, 18,
        18, 19, 16,
        
        20, 21, 22,
        22, 23, 20,
    };
    
    glGenVertexArrays(1, &map.vao);
    glBindVertexArray(map.vao);
    
    glGenBuffers(1, &map.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, map.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glGenBuffers(1, &map.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, map.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    return map;
}

static void
cubemap_draw_skybox(Cubemap skybox)
{
    glBindVertexArray(skybox.vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture);
    
    glDepthFunc(GL_LEQUAL);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
    
    glDepthFunc(GL_LESS);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
    
    element.program = program_create_from_file(UI_VERTEX_FILENAME, UI_FRAG_FILENAME);
    element.texture = texture_create_from_file(texture_filename);
    
    glBindTexture(GL_TEXTURE_2D, element.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return element;
}

static void
ui_element_draw(UIElement element)
{
    // NOTE: We could bunch up all of the UI elements together and bind the shader program
    // once. That would be more efficient
    glUseProgram(element.program);
    Mat4 transform = translate(Mat4(1.0f), Vec3(element.position.x, element.position.y, 0.0f));
    transform = scale(transform, Vec3(element.size.x, element.size.y, 1.0f));
    opengl_set_uniform(element.program, "transform", transform);
    
    Mat4 ortho = create_ortographic(1280.0f/720.0f, 0.01f, 10.0f);
    opengl_set_uniform(element.program, "ortho", ortho);
    
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    
    glBindVertexArray(element.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, element.texture);
    glUniform1i(glGetUniformLocation(element.program, "tex_sampler"), 0);
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
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        Mesh *mesh = &model->meshes[i];
        
        for(u32 t = 0; t < mesh->indices_len; t += 3)
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
    for(u32 t = 0; t < mesh->indices_len; t += 3)
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
    for(u32 i = 0; i < entity->model->hitboxes_len; i++)
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

static GLuint
program_create_from_file(const char *vertex_filename, const char *fragment_filename)
{
    FILE *f = fopen(vertex_filename, "r");
    assert(f);
    char *vertex_src = read_file_to_string(f);
    fclose(f);
    
    f = fopen(fragment_filename, "r");
    assert(f);
    char *fragment_src = read_file_to_string(f);
    fclose(f);
    
    GLuint program = program_create(vertex_src, fragment_src);
    
    return program;
}

static BasicShaderProgram
basic_program_build()
{
    BasicShaderProgram result = {};
    
    result.id = program_create_from_file(MAIN_VERTEX_FILENAME, MAIN_FRAG_FILENAME);
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
    result.material_ambient_component = glGetUniformLocation(result.id, "material.ambient_component");
    result.material_diffuse_component = glGetUniformLocation(result.id, "material.diffuse_component");
    result.material_specular_component = glGetUniformLocation(result.id, "material.specular_component");
    result.material_specular_exponent = glGetUniformLocation(result.id, "material.specular_exponent");
    
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