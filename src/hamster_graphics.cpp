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
                    FLAG_SET(current_mesh->flags, OBJ_MESH_FLAG_FACE_HAS_VERTEX);
                }
                if(face_parts == 2 || face_set == 3) {
                    FLAG_SET(current_mesh->flags, OBJ_MESH_FLAG_FACE_HAS_TEXTURE);
                }
                if(face_parts >= 3) {
                    FLAG_SET(current_mesh->flags, OBJ_MESH_FLAG_FACE_HAS_NORMAL);
                }
                
                // NOTE(mateusz): For minus one see note above, for minus two we are getting the number of triangles for this face.
                triangles_count += parts - 1 - 2;
            }
        }
    }
    
    free(contents);
    
    if(flags & OBJ_PARSE_FLAG_FLIP_UVS)
    {
        for(u32 i = 0; i < model.meshes_len; i++)
        {
            OBJMesh *mesh = &model.meshes[i];
            for(u32 j = 0; j < mesh->vertices_len; j++)
            {
                mesh->texture_uvs[j].y = 1.0f - mesh->texture_uvs[j].y;
            }
        }
    }
    
    if(flags & OBJ_PARSE_FLAG_GEN_TANGENTS)
    {
        for(u32 i = 0; i < model.meshes_len; i++)
        {
            OBJMesh *mesh = &model.meshes[i];
            mesh->tangents = (Vec3 *)malloc(mesh->vertices_len * sizeof(Vec3));
            if(flags & OBJ_PARSE_FLAG_GEN_BITANGETS)
            {
                mesh->bitangents = (Vec3 *)malloc(mesh->vertices_len * sizeof(Vec3));
            }
            
            for(u32 j = 0; j < mesh->indices_len; j += 3)
            {
                u32 id0 = mesh->indices[j + 0];
                u32 id1 = mesh->indices[j + 1];
                u32 id2 = mesh->indices[j + 2];
                
                Vec3 pos_v0 = mesh->vertexes[id0];
                Vec3 pos_v1 = mesh->vertexes[id1];
                Vec3 pos_v2 = mesh->vertexes[id2];
                
                Vec2 uv_v0 = mesh->texture_uvs[id0];
                Vec2 uv_v1 = mesh->texture_uvs[id1];
                Vec2 uv_v2 = mesh->texture_uvs[id2];
                
                Vec3 delta_pos0 = sub(pos_v1, pos_v0);
                Vec3 delta_pos1 = sub(pos_v2, pos_v0);
                Vec2 delta_uv0  = sub(uv_v1, uv_v0);
                Vec2 delta_uv1  = sub(uv_v2, uv_v0);
                
                f32 rdet = 1.0f / (delta_uv0.x * delta_uv1.y - delta_uv0.y * delta_uv1.x);
                Vec3 tangent = sub(scale(delta_pos0, delta_uv1.y), scale(delta_pos1, delta_uv0.y));
                tangent = scale(tangent, rdet);
                mesh->tangents[id0] = mesh->tangents[id1] = mesh->tangents[id2] = tangent;
                
                if(flags & OBJ_PARSE_FLAG_GEN_BITANGETS)
                {
                    Vec3 bitangent = add(scale(delta_pos0, -delta_uv1.x), scale(delta_pos1, delta_uv0.x));
                    bitangent = scale(bitangent, rdet);
                    mesh->bitangents[id0] = mesh->bitangents[id1] = mesh->bitangents[id2] = bitangent;
                }
            }
        }
    }
    
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
    
    free(vertices);
    free(texture_uvs);
    free(normals);
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
        free(model->meshes[i].bitangents);
        free(model->meshes[i].indices);
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
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            FLAG_SET(new_material->flags, MATERIAL_FLAGS_HAS_DIFFUSE_MAP);
        }
        
        if(!string_empty(obj_mtl->specular_map_filename))
        {
            strcpy(texture_filename + path_length, obj_mtl->specular_map_filename);
            new_material->specular_map = texture_create_from_file(texture_filename);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            FLAG_SET(new_material->flags, MATERIAL_FLAGS_HAS_SPECULAR_MAP);
        }
        
        if(!string_empty(obj_mtl->normal_map_filename))
        {
            strcpy(texture_filename + path_length, obj_mtl->normal_map_filename);
            new_material->normal_map = texture_create_from_file(texture_filename);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            FLAG_SET(new_material->flags, MATERIAL_FLAGS_HAS_NORMAL_MAP);
        }
    }
}

static Model
model_create_debug_floor()
{
    Vertices vs = {};
    vs.positions = (Vec3 *)malloc(4 * sizeof(Vec3));
    vs.texture_uvs = (Vec2 *)malloc(4 * sizeof(Vec2));
    vs.normals = (Vec3 *)malloc(4 * sizeof(Vec3));
    
    vs.positions[0] = Vec3(-1.0f, 0.0f, 1.0f);
    vs.positions[1] = Vec3(1.0f, 0.0f, 1.0f);
    vs.positions[2] = Vec3(1.0f, 0.0f, -1.0f);
    vs.positions[3] = Vec3(-1.0f, 0.0f, -1.0f);
    
    vs.texture_uvs[0] = Vec2(0.0f, 0.0f);
    vs.texture_uvs[1] = Vec2(1.0f, 0.0f);
    vs.texture_uvs[2] = Vec2(1.0f, 1.0f);
    vs.texture_uvs[3] = Vec2(0.0f, 1.0f);
    
    vs.normals[0] = Vec3(0.0f, 1.0f, 0.0f);
    vs.normals[1] = Vec3(0.0f, 1.0f, 0.0f);
    vs.normals[2] = Vec3(0.0f, 1.0f, 0.0f);
    vs.normals[3] = Vec3(0.0f, 1.0f, 0.0f);
    
    u32 indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    Model model = {};
    model.materials = (Material *)malloc(sizeof(Material)); model.materials[0] = {};
    model.materials_len = 1;
    
    Material *mat = model.materials;
    const char *mat_name = "Default";
    strcpy(mat->name, mat_name);
    
    mat->specular_exponent = 1.0f;
    Vec3 one = Vec3(1.0f, 1.0f, 1.0f);
    mat->ambient_component = mat->diffuse_component = mat->specular_component = one;
    
    mat->diffuse_map = texture_create_from_file("data/wood.png");
    FLAG_SET(mat->flags, MATERIAL_FLAGS_HAS_DIFFUSE_MAP);
    
    model.meshes = (Mesh *)malloc(sizeof(Mesh));
    model.meshes[model.meshes_len++] = {};
    Mesh *mesh = model.meshes;
    
    mesh->vertices = vs;
    mesh->vertices_len = 4;
    strcpy(mesh->material_name, mat_name);
    
    mesh->indices = (u32 *)malloc(sizeof(indices));
    for(u32 i = 0; i < ARRAY_LEN(indices); i++)
    {
        mesh->indices[mesh->indices_len++] = indices[i];
    }
    
    model_finalize_mesh(mesh);
    
    return model;
}

static Model
model_create_from_obj(OBJModel *obj)
{
    Model model = {};
    
    OBJMesh *objmesh = NULL;
    model.meshes = (Mesh *)malloc(obj->meshes_len * sizeof(Mesh));
    model.hitboxes = (Hitbox *)malloc(obj->meshes_len * sizeof(Hitbox));
    for(u32 i = 0; i < obj->meshes_len; i++)
    {
        objmesh = &obj->meshes[i];
        
        model.meshes[model.meshes_len++] = {};
        Mesh *mesh = &model.meshes[model.meshes_len - 1];
        
        strcpy(mesh->material_name, objmesh->mtl_name);
        mesh->vertices = {};
        bool uvs = objmesh->texture_uvs;
        bool normals = objmesh->normals;
        bool tangents = objmesh->tangents;
        bool bitangents = objmesh->bitangents;
        
        mesh->vertices.positions = (Vec3 *)malloc(objmesh->vertices_len * sizeof(Vec3));
        
        if(uvs)
            mesh->vertices.texture_uvs = (Vec2 *)malloc(objmesh->vertices_len * sizeof(Vec2));
        if(normals)
            mesh->vertices.normals = (Vec3 *)malloc(objmesh->vertices_len * sizeof(Vec3));
        if(tangents)
            mesh->vertices.tangents = (Vec3 *)malloc(objmesh->vertices_len * sizeof(Vec3));
        if(bitangents)
            mesh->vertices.bitangents = (Vec3 *)malloc(objmesh->vertices_len * sizeof(Vec3));
        
        mesh->vertices_len = objmesh->vertices_len;
        for(unsigned int j = 0; j < objmesh->vertices_len; ++j)
        {
            mesh->vertices.positions[j] = objmesh->vertexes[j];
            if(uvs)
                mesh->vertices.texture_uvs[j] = objmesh->texture_uvs[j];
            if(normals)
                mesh->vertices.normals[j] = objmesh->normals[j];
            if(tangents)
                mesh->vertices.tangents[j] = objmesh->tangents[j];
            if(bitangents)
                mesh->vertices.bitangents[j] = objmesh->bitangents[j];
        }
        
        mesh->indices = (u32 *)malloc(objmesh->indices_len * sizeof(u32));
        for(u32 j = 0; j < objmesh->indices_len; j++)
        {
            mesh->indices[mesh->indices_len++] = objmesh->indices[j];
        }
        
        model.hitboxes[model.hitboxes_len++] = hitbox_create_from_mesh(mesh);
        model_finalize_mesh(mesh);
    }
    
    FLAG_SET(model.flags, MODEL_FLAGS_MESH_NORMALS_SHADED);
    FLAG_UNSET(model.flags, MODEL_FLAGS_GOURAUD_SHADED);
    
    return model;
}

static void 
model_finalize_mesh(Mesh *mesh)
{
    bool uvs = mesh->vertices.texture_uvs != NULL;
    bool normals = mesh->vertices.normals != NULL;
    bool tangents = mesh->vertices.tangents != NULL;
    bool bitangents = mesh->vertices.bitangents != NULL;
    
    u32 stride = sizeof(*mesh->vertices.positions);
    stride += uvs ? sizeof(*mesh->vertices.texture_uvs) : 0;
    stride += normals ? sizeof(*mesh->vertices.normals) : 0;
    stride += tangents ? sizeof(*mesh->vertices.tangents) : 0;
    stride += bitangents ? sizeof(*mesh->vertices.bitangents) : 0;
    
    u32 data_len = 0;
    u32 size = mesh->vertices_len * stride;
    f32 *data = (f32 *)malloc(size);
    for(u32 i = 0; i < mesh->vertices_len; i++)
    {
        data[data_len++] = mesh->vertices.positions[i].x;
        data[data_len++] = mesh->vertices.positions[i].y;
        data[data_len++] = mesh->vertices.positions[i].z;
        
        if(uvs)
        {
            data[data_len++] = mesh->vertices.texture_uvs[i].x;
            data[data_len++] = mesh->vertices.texture_uvs[i].y;
        }
        if(normals)
        {
            data[data_len++] = mesh->vertices.normals[i].x;
            data[data_len++] = mesh->vertices.normals[i].y;
            data[data_len++] = mesh->vertices.normals[i].z;
        }
        if(tangents)
        {
            data[data_len++] = mesh->vertices.tangents[i].x;
            data[data_len++] = mesh->vertices.tangents[i].y;
            data[data_len++] = mesh->vertices.tangents[i].z;
        }
        if(bitangents)
        {
            data[data_len++] = mesh->vertices.bitangents[i].x;
            data[data_len++] = mesh->vertices.bitangents[i].y;
            data[data_len++] = mesh->vertices.bitangents[i].z;
        }
    }
    
    assert(mesh->indices_len != 0);
    if(mesh->vao == 0)
    {
        glGenVertexArrays(1, &mesh->vao);
        glGenBuffers(1, &mesh->vbo);
        glGenBuffers(1, &mesh->ebo);
    }
    
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    free(data);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices_len * sizeof(u32), mesh->indices, GL_STATIC_DRAW);
    
    size_t offset = 0;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offset);
    offset += sizeof(Vec3);
    
    if(uvs)
    {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)offset);
        offset += sizeof(Vec2);
    }
    
    if(normals)
    {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void *)offset);
        offset += sizeof(Vec3);
    }
    
    if(tangents)
    {
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void *)offset);
        offset += sizeof(Vec3);
    }
    
    if(bitangents)
    {
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void *)offset);
        offset += sizeof(Vec3);
    }
    
    assert(offset == stride);
}

static void 
model_destory(Model model)
{
    for(u32 i = 0; i < model.meshes_len; i++)
    {
        free(model.meshes[i].indices);
        free(model.meshes[i].vertices.positions);
        free(model.meshes[i].vertices.texture_uvs);
        free(model.meshes[i].vertices.normals);
        free(model.meshes[i].vertices.tangents);
        free(model.meshes[i].vertices.bitangents);
        glDeleteVertexArrays(1, &model.meshes[i].vao);
        glDeleteBuffers(1, &model.meshes[i].vbo);
        glDeleteBuffers(1, &model.meshes[i].ebo);
    }
    
    for(u32 i = 0; i < model.materials_len; i++)
    {
        glDeleteTextures(1, &model.materials[i].diffuse_map);
        glDeleteTextures(1, &model.materials[i].specular_map);
        glDeleteTextures(1, &model.materials[i].normal_map);
    }
    
    free(model.meshes);
    free(model.hitboxes);
    free(model.materials);
}

// Takes a model and recomputes the normals to be smoothed gouraud style
static void
model_gouraud_shade(Model *model)
{
#if 0
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
#endif
    model->flags = (ModelFlags)(model->flags | MODEL_FLAGS_GOURAUD_SHADED);
    model->flags = (ModelFlags)(model->flags & ~MODEL_FLAGS_MESH_NORMALS_SHADED);
}

static void
model_mesh_normals_shade(Model *model)
{
#if 0
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        Mesh *mesh = &model->meshes[0];
        
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->vertices_len * sizeof(mesh->vertices[0]), mesh->vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
#endif
    
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
        maxpoint.x = MAX(mesh->vertices.positions[i].x, maxpoint.x);
        maxpoint.y = MAX(mesh->vertices.positions[i].y, maxpoint.y);
        maxpoint.z = MAX(mesh->vertices.positions[i].z, maxpoint.z);
        
        minpoint.x = MIN(mesh->vertices.positions[i].x, minpoint.x);
        minpoint.y = MIN(mesh->vertices.positions[i].y, minpoint.y);
        minpoint.z = MIN(mesh->vertices.positions[i].z, minpoint.z);	
    }
    
    Hitbox result = {};
    result.refpoint = minpoint;
    result.size = abs(sub(maxpoint, minpoint));
    
    return result;
}

static Hitbox
hitbox_as_cylinder(Line line, f32 r)
{
    Hitbox result = {};
    
    result.refpoint = subs(line.point0, r);
    result.size = adds(sub(line.point1, line.point0), r);
    
    return result;
}

static bool
hitbox_in_frustum(Hitbox *hbox, Plane *planes, Mat4 transform)
{
    Vec4 s4 = mul(transform, Vec4(hbox->size, 0.0f));
    Vec3 halfsize = scale(Vec3(s4.x, s4.y, s4.z), 0.5f);
    f32 r = sqrtf(square(halfsize.x) + square(halfsize.y) + square(halfsize.z));
    Vec3 point = add(mul(transform, hbox->refpoint), halfsize);
    
    for(u32 i = 0; i < FrustumPlane_ElementCount; i++)
    {
        if(inner(point, planes[i].normal) + planes[i].d + r <= 0)
        {
            return false;
        }
    }
    
    return true;
}

static Vec2
world_point_to_screen(Vec3 world_point, Mat4 proj, Mat4 view)
{
    Vec3 result = {};
    
    result = mul(proj, mul(view, world_point));
    result = scale(result, 1.0f / result.z); // Perspective division.
    
    return Vec2(result.x, result.y);
}

static Vec3
ndc_to_ray_direction(Vec2 ndc, Mat4 proj_inversed, Mat4 view_inversed)
{
    Vec3 result = {};
    
    Vec4 clip_space = Vec4(ndc.x, ndc.y, -1.0f, 1.0f);
    Vec4 eye_space = mul(proj_inversed, clip_space);
    eye_space = Vec4(eye_space.x, eye_space.y, -1.0f, 0.0f);
    Vec4 world_space = mul(view_inversed, eye_space);
    
    result = noz(Vec3(world_space.x, world_space.y, world_space.z));
    
    return result;
}

static Vec2
screen_to_ndc(f32 xmouse, f32 ymouse, f32 width, f32 height)
{
    Vec2 result = {};
    
    result.x = (2.0f * xmouse) / width - 1.0f;
    result.y = (2.0f * ((f64)height - ymouse)) / height - 1.0f;
    
    return result;
}

static Vec2
screen_to_ndc(Vec2 mouse, Vec2 win_dim)
{
    Vec2 result = {};
    
    result = screen_to_ndc(mouse.x, mouse.y, win_dim.x, win_dim.y);
    
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
    
    return map;
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
    
    element.texture = texture_create_from_file(texture_filename);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    return element;
}

// Based on:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
static bool
ray_intersect_triangle(Vec3 ray_origin, Vec3 ray_direction,
                       Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal, f32 *dist_result)
{
    f32 zero_treshold = 0.0001f;
    f32 denom = inner(normal, ray_direction);
    if(denom < zero_treshold && denom > -zero_treshold) { return false; }
    
    f32 dist = -inner(normal, v0);
    f32 ray_dist = -(inner(normal, ray_origin) + dist) / denom;
    if(ray_dist < zero_treshold && ray_dist > -zero_treshold) { return false; }
    \
    Vec3 edge0 = sub(v1, v0);
    Vec3 edge1 = sub(v2, v1);
    Vec3 edge2 = sub(v0, v2);
    Vec3 plane_hit = add(ray_origin, scale(ray_direction, ray_dist));
    
    Vec3 c0 = sub(plane_hit, v0); 
    Vec3 c1 = sub(plane_hit, v1);
    Vec3 c2 = sub(plane_hit, v2);
    bool hit = inner(normal, cross(edge0, c0)) > 0.0f && inner(normal, cross(edge1, c1)) > 0.0f &&
        inner(normal, cross(edge2, c2)) > 0.0f;
    
    if(hit) {
        *dist_result = ray_dist;
    }
    
    return hit;
}

static bool
ray_intersect_triangle(Vec3 ray_origin, Vec3 ray_direction,
                       Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal)
{
    f32 tmp;
    
    return ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, &tmp);
}

static bool
ray_intersect_model(Vec3 ray_origin, Vec3 ray_direction, Model *model)
{
    for(u32 i = 0; i < model->meshes_len; i++)
    {
        Mesh *mesh = &model->meshes[i];
        
        for(u32 t = 0; t < mesh->indices_len; t += 3)
        {
            Vec3 normal = mesh->vertices.normals[mesh->indices[t + 0]];
            assert(normal == mesh->vertices.normals[mesh->indices[t + 1]] &&
                   normal == mesh->vertices.normals[mesh->indices[t + 2]]);
            
            Vec3 v0 = mesh->vertices.positions[mesh->indices[t + 0]];
            Vec3 v1 = mesh->vertices.positions[mesh->indices[t + 1]];
            Vec3 v2 = mesh->vertices.positions[mesh->indices[t + 2]];
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
        Vec3 v0 = mul(transform, mesh->vertices.positions[mesh->indices[t + 0]]);
        Vec3 v1 = mul(transform, mesh->vertices.positions[mesh->indices[t + 1]]);
        Vec3 v2 = mul(transform, mesh->vertices.positions[mesh->indices[t + 2]]);
        
        Vec3 normal0 = mesh->vertices.normals[mesh->indices[t + 0]];
        Vec3 normal1 = mesh->vertices.normals[mesh->indices[t + 1]];
        Vec3 normal2 = mesh->vertices.normals[mesh->indices[t + 2]];
        
        // NOTE(mateusz): I don't know if this is going to be faster,
        // but my guess is that it's going to be, because either the
        // model is smoothed or not, so it will be basicly free.
        Vec3 normal = {};
        if(normal0 == normal1 && normal1 == normal2) {
            normal = normal0;
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
ray_intersect_hitbox(Vec3 ray_origin, Vec3 ray_direction, Hitbox *hbox, f32 *dist_result)
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
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    v0 = vertices[6];
    v1 = vertices[5];
    v2 = vertices[7];
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    // Back face
    v0 = vertices[0];
    v1 = vertices[1];
    v2 = vertices[2];
    normal = triangle_normal(v0, v1, v2);
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    v0 = vertices[1];
    v1 = vertices[4];
    v2 = vertices[2];
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    // Left face
    v0 = vertices[0];
    v1 = vertices[2];
    v2 = vertices[3];
    normal = triangle_normal(v0, v1, v2);
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    v2 = vertices[3];
    v1 = vertices[2];
    v0 = vertices[5];
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    // Right face
    v0 = vertices[1];
    v1 = vertices[4];
    v2 = vertices[6];
    normal = triangle_normal(v0, v1, v2);
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    v0 = vertices[6];
    v1 = vertices[4];
    v2 = vertices[7];
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    // Top face
    v0 = vertices[2];
    v1 = vertices[5];
    v2 = vertices[4];
    normal = triangle_normal(v0, v1, v2);
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    v0 = vertices[4];
    v1 = vertices[5];
    v2 = vertices[7];
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    // Bottom face
    v0 = vertices[0];
    v1 = vertices[1];
    v2 = vertices[3];
    normal = triangle_normal(v0, v1, v2);
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    v0 = vertices[1];
    v1 = vertices[3];
    v2 = vertices[6];
    if(ray_intersect_triangle(ray_origin, ray_direction, v0, v1, v2, normal, dist_result)) { return true; }
    
    return false;
}


static bool
ray_intersect_hitbox(Vec3 ray_origin, Vec3 ray_direction, Hitbox *hbox)
{
    f32 tmp;
    
    return ray_intersect_hitbox(ray_origin, ray_direction, hbox, &tmp);
}

// NOTE(mateusz): This is in no way a final function, it should really be called
// debug somehow. Real intersections will be working a bit different. //// HOW???
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

static bool
click_result_swap_func(void *elem0, void *elem1)
{
    AxisClickResult *c0 = (AxisClickResult *)elem0;
    AxisClickResult *c1 = (AxisClickResult *)elem1;
    
    if(c0->clicked && c1->clicked)
    {
        return c0->distance > c1->distance;
    }
    else if(c0->clicked)
    {
        return false;
    }
    else if(c1->clicked)
    {
        return true;
    }
    
    return true;
}

static AxisClickResult 
closest_click_result(AxisClickResult xaxis, AxisClickResult yaxis, AxisClickResult zaxis)
{
    AxisClickResult closest = {};
    
    AxisClickResult axis[] = {
        xaxis, yaxis, zaxis,
    };
    
    u32 clicked_count = 0;
    for(u32 i = 0; i < ARRAY_LEN(axis); i++)
    {
        clicked_count += axis[i].clicked ? 1 : 0;
    }
    
    if(clicked_count == 1)
    {
        for(u32 i = 0; i < ARRAY_LEN(axis); i++)
        {
            if(axis[i].clicked)
            {
                closest = axis[i];
                break;
            }
        }
    }
    else
    {
        sort(axis, ARRAY_LEN(axis), sizeof(axis[0]), click_result_swap_func);
        
        closest = axis[0];
    }
    
    return closest;
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
    
    return texture;
}
