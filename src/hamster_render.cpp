#include "hamster_render.h"

static RenderQueue
render_create_queue(u32 size)
{
    RenderQueue queue = {};
    
    queue.max_size = size;
    queue.entries = malloc(queue.max_size);
    
    return queue;
}

static void 
render_destory_queue(RenderQueue queue)
{
    free(queue.entries);
}

static void
*_render_push_entry(RenderQueue *queue, u32 struct_size, RenderHeader type)
{
    void *result = 0;
    
    assert(queue->max_size > queue->size + struct_size);
    
    result = (void *)((u8 *)queue->entries + queue->size);
    queue->size += struct_size;
    queue->len += 1;
    
    *(RenderHeader *)result = type;
    
    return result;
}

static void 
render_push_skybox(RenderQueue *queue, Cubemap skybox)
{
    RenderEntrySkybox *entry = render_push_entry(queue, RenderEntrySkybox);
    
    entry->cube = skybox;
}

static void 
render_flush(RenderQueue *queue, RenderContext *ctx)
{
    RenderHeader *header = (RenderHeader *)queue->entries;
    
    for(u32 i = 0; i < queue->len; i++)
    {
        switch(*header)
        {
            case RenderType_RenderEntrySkybox:
            {
                GLuint program_id = ctx->programs[ShaderProgram_Skybox].id;
                
                glUseProgram(program_id);
                Mat4 view_no_translation = ctx->lookat;
                view_no_translation.columns[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
                opengl_set_uniform(program_id, "view", view_no_translation);
                opengl_set_uniform(program_id, "proj", ctx->proj);
                
                RenderEntrySkybox *entry = (RenderEntrySkybox *)header;
                glBindVertexArray(entry->cube.vao);
                glBindTexture(GL_TEXTURE_CUBE_MAP, entry->cube.texture);
                
                glDepthFunc(GL_LEQUAL);
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
                glDepthFunc(GL_LESS);
                
                header = (RenderHeader *)(++entry);
            } break;
        }
    }
    
    queue->len = 0;
    queue->size = 0;
    memset(queue->entries, 0, queue->max_size);
}

static void
render_load_programs(RenderContext *ctx)
{
    ctx->programs[ShaderProgram_Basic] = program_create_from_file(MAIN_VERTEX_FILENAME, MAIN_FRAG_FILENAME);
    ctx->programs[ShaderProgram_Simple] = program_create_from_file(SIMPLE_VERTEX_FILENAME, SIMPLE_FRAG_FILENAME);
    ctx->programs[ShaderProgram_Skybox] = program_create_from_file(SKYBOX_VERTEX_FILENAME, SKYBOX_FRAG_FILENAME);
    ctx->programs[ShaderProgram_UI] = program_create_from_file(UI_VERTEX_FILENAME, UI_FRAG_FILENAME);
    ctx->programs[ShaderProgram_Line] = program_create_from_file(MAIN_VERTEX_FILENAME, LINE_FRAG_FILENAME);
}

// Takes a compiled shader, checks if it produced an error
// returns false if it did, true otherwise.
static bool
program_shader_ok(GLuint shader)
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
program_ok(GLuint program)
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

static ShaderProgram
program_create_from_file(const char *vertex_filename, const char *fragment_filename)
{
    ShaderProgram program = {};
    program.vertex_filename = vertex_filename;
    program.fragment_filename = fragment_filename;
    
    program.vertex_stamp = get_file_stamp(vertex_filename);
    program.fragment_stamp = get_file_stamp(fragment_filename);
    
    FILE *f = fopen(vertex_filename, "r");
    assert(f);
    char *vertex_src = read_file_to_string(f);
    fclose(f);
    
    f = fopen(fragment_filename, "r");
    assert(f);
    char *fragment_src = read_file_to_string(f);
    fclose(f);
    
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(vertex_shader);
    if(!program_shader_ok(vertex_shader))
    {
        printf("Error in file [%s]\n", vertex_filename);
    }
    
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(fragment_shader);
    if(!program_shader_ok(fragment_shader))
    {
        printf("Error in file [%s]\n", fragment_filename);
    }
    
    program.id = glCreateProgram();
    glAttachShader(program.id, vertex_shader);
    glAttachShader(program.id, fragment_shader);
    glLinkProgram(program.id);
    assert(program_ok(program.id));
    
    free(vertex_src);
    free(fragment_src);
    
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
opengl_set_uniform(GLuint program, const char *name, i32 v)
{
    GLint location = glGetUniformLocation(program, name);
    glUniform1i(location, v);
    assert(glGetError() == GL_NO_ERROR);
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
