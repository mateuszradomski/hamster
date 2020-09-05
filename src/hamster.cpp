#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>

// NOTE(mateusz): Unix systems only!
#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <x86intrin.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <libs/imgui/imgui.h>
#include <libs/imgui/imgui_impl_glfw.h>
#include <libs/imgui/imgui_impl_opengl3.h>

// TODO: As a learning expierience try to implement png reading yourself.
// PNG reading is quite a chore, just because of the zlib compressed IDAT chunks,
// maybe just use zlib and decode the rest yourself.
#define STB_IMAGE_IMPLEMENTATION
#include "includes/stb_image.h"

#include "hamster_math.cpp"
#include "hamster_util.cpp"
#include "hamster_graphics.cpp"
#include "hamster_render.cpp"
#include "hamster.h"

void
opengl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
					  GLsizei length, const GLchar *message, const void *userParam)
{
	NOT_USED(source);
	NOT_USED(type);
	NOT_USED(id);
	NOT_USED(severity);
	NOT_USED(length);
	NOT_USED(userParam);
	
	printf("OpenGL Error: %s\n", message);
	assert(false);
}

static Window
create_opengl_window()
{
	assert(glfwInit());
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	Window window;
	window.ptr = glfwCreateWindow(window.width, window.height, "hamster", NULL, NULL);
	glfwMakeContextCurrent(window.ptr);
	assert(glewInit() == 0); // That means no errors
	
	return window;
}

static void
window_debug_title(Window *window, Timer timer)
{
    f64 frame_time = timer.frame_end - timer.frame_start;
    
    char title[128] = {};
    sprintf(title, "hamster | %d fps | %f ms frame", timer.frames, frame_time);
    glfwSetWindowTitle(window->ptr, title);
}

// TODO: I don't know if this keyboard button callback is going to stay because
// it's really unresponsive. There is a delay that you can definitley feel from
// pressing a button and it registering as being down. Might switch to polling
// all the necesseray keys for greater smoothness. -radomski
static void
keyboard_button_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	assert(key != GLFW_KEY_UNKNOWN);
	ProgramState *state = (ProgramState *)glfwGetWindowUserPointer(window);
	NOT_USED(mods); // NOTE: I guess we would use it somewhere??
    NOT_USED(scancode); // That is unused.
    
    if(action == GLFW_PRESS) {
        state->kbuttons[key].down = true;
    } else if(action == GLFW_RELEASE) {
        state->kbuttons[key].down = false;
    }
}

static void
mouse_button_callback(GLFWwindow *window, int key, int action, int mods)
{
	assert(key != GLFW_KEY_UNKNOWN);
	ProgramState *state = (ProgramState *)glfwGetWindowUserPointer(window);
	NOT_USED(mods); // NOTE: I guess we would use it somewhere??
	
	if(action == GLFW_PRESS) {
		state->mbuttons[key].down = true;
	} else {
		state->mbuttons[key].down = false;
	}
}

static void
buttons_update(Button *buttons, u32 length)
{
	for(u32 i = 0; i < length; i++)
	{
		buttons[i].pressed = buttons[i].down && !buttons[i].last;
		buttons[i].last = buttons[i].down;
	}
}

int main()
{
	ProgramState state = {};
	state.window = create_opengl_window();
	glfwSetWindowUserPointer(state.window.ptr, &state);
	glfwSetKeyCallback(state.window.ptr, keyboard_button_callback);
	glfwSetMouseButtonCallback(state.window.ptr, mouse_button_callback);
	
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(opengl_error_callback, 0);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    
    OBJParseFlags flags = OBJ_PARSE_FLAG_EMPTY;
    FLAG_SET(flags, OBJ_PARSE_FLAG_GEN_TANGENTS, OBJParseFlags);
    FLAG_SET(flags, OBJ_PARSE_FLAG_GEN_BITANGETS, OBJParseFlags);
    FLAG_SET(flags, OBJ_PARSE_FLAG_FLIP_UVS, OBJParseFlags);
    f64 start = glfwGetTime();
    const char *filename = "data/model.obj";
	OBJModel obj = obj_parse(filename, flags);
    printf("[%s] parsed in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
	Model monkey_model = model_create_from_obj(&obj);
    printf("[%s] loaded in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
    model_load_obj_materials(&monkey_model, obj.materials, obj.materials_len, filename);
    printf("[%s] materials loaded in %f\n\n", filename, glfwGetTime() - start);
    obj_model_destory(&obj);
    
    FLAG_UNSET(flags, OBJ_PARSE_FLAG_FLIP_UVS, OBJParseFlags);
    start = glfwGetTime();
    filename = "data/backpack/backpack.obj";
    obj = obj_parse(filename, flags);
    printf("[%s] parsed in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
	Model backpack_model = model_create_from_obj(&obj);
    printf("[%s] loaded in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
    model_load_obj_materials(&backpack_model, obj.materials, obj.materials_len, filename);
    printf("[%s] materials loaded in %f\n\n", filename, glfwGetTime() - start);
    obj_model_destory(&obj);
    
    monkey_model.texture = backpack_model.texture = texture_create_solid(1.0f, 1.0f, 1.0f, 1.0f);
    
    flags = OBJ_PARSE_FLAG_EMPTY;
    FLAG_SET(flags, OBJ_PARSE_FLAG_GEN_TANGENTS, OBJParseFlags);
    FLAG_SET(flags, OBJ_PARSE_FLAG_GEN_BITANGETS, OBJParseFlags);
    FLAG_SET(flags, OBJ_PARSE_FLAG_FLIP_UVS, OBJParseFlags);
    start = glfwGetTime();
    filename = "data/nanosuit/nanosuit.obj";
    obj = obj_parse(filename, flags);
    printf("[%s] parsed in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
	Model crysis_model = model_create_from_obj(&obj);
    printf("[%s] loaded in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
    model_load_obj_materials(&crysis_model, obj.materials, obj.materials_len, filename);
    crysis_model.texture = monkey_model.texture;
    printf("[%s] materials loaded in %f\n\n", filename, glfwGetTime() - start);
    obj_model_destory(&obj);
    
    flags = OBJ_PARSE_FLAG_EMPTY;
    FLAG_SET(flags, OBJ_PARSE_FLAG_GEN_TANGENTS, OBJParseFlags);
    FLAG_SET(flags, OBJ_PARSE_FLAG_GEN_BITANGETS, OBJParseFlags);
    FLAG_SET(flags, OBJ_PARSE_FLAG_FLIP_UVS, OBJParseFlags);
    start = glfwGetTime();
    filename = "data/cyborg/cyborg.obj";
    obj = obj_parse(filename, flags);
    printf("[%s] parsed in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
	Model cyborg_model = model_create_from_obj(&obj);
    printf("[%s] loaded in %f\n", filename, glfwGetTime() - start);
    start = glfwGetTime();
    model_load_obj_materials(&cyborg_model, obj.materials, obj.materials_len, filename);
    cyborg_model.texture = monkey_model.texture;
    printf("[%s] materials loaded in %f\n\n", filename, glfwGetTime() - start);
    obj_model_destory(&obj);
    
	Model floor_model = model_create_debug_floor();
	UIElement crosshair = ui_element_create(Vec2(0.0f, 0.0f), Vec2(0.1f, 0.1f),
                                            "data/crosshair.png");
    crosshair.program = program_create_from_file(UI_VERTEX_FILENAME, UI_FRAG_FILENAME).id;
	Cubemap skybox = cubemap_create_skybox();
    GLuint skybox_program = program_create_from_file(SKYBOX_VERTEX_FILENAME, SKYBOX_FRAG_FILENAME).id;
    NOT_USED(skybox_program);
    
    Entity monkey = {};
	monkey.position = Vec3(5.0f, 0.0f, 0.0f);
	monkey.size = Vec3(1.0f, 1.0f, 1.0f);
	monkey.model = &monkey_model;
    
    Entity backpack = {};
    backpack.position = Vec3(0.0f, 1.0f, -1.0f);
    backpack.size = Vec3(1.0f, 1.0f, 1.0f);
    backpack.model = &backpack_model;
    
    Entity crysis_guy = {};
    crysis_guy.position = Vec3(-5.0f, -1.0f, 0.0f);
    crysis_guy.size = Vec3(0.25f, 0.25f, 0.25f);
    crysis_guy.model = &crysis_model;
    
    Entity cyborg = {};
    cyborg.position = Vec3(-7.5f, -1.0f, 2.0f);
    cyborg.size = Vec3(1.0f, 1.0f, 1.0f);
    cyborg.model = &cyborg_model;
    
    Entity floor = {};
	floor.position = Vec3(0.0f, -2.0f, 0.0f);
	floor.size = Vec3(10.0f, 1.0f, 10.0f);
	floor.model = &floor_model;
	
    GLuint main_program = program_create_from_file(MAIN_VERTEX_FILENAME, MAIN_FRAG_FILENAME).id;
    //GLuint line_program = program_create_from_file(MAIN_VERTEX_FILENAME, LINE_FRAG_FILENAME).id;
    GLuint simple_program = program_create_from_file(SIMPLE_VERTEX_FILENAME, SIMPLE_FRAG_FILENAME).id;
    
    RenderContext ctx_ = {};
    RenderContext *ctx = &ctx_;
    render_load_programs(ctx);
    
	ctx->cam.position = Vec3(0.0f, 0.0f, 3.0f);
	ctx->cam.yaw = asinf(-1.0f); // Where we look
	camera_calculate_vectors(&ctx->cam);
	
    RenderQueue rqueue_ = render_create_queue();
    RenderQueue *rqueue = &rqueue_;
    
    Mat4 lookat = look_at(ctx->cam.front, ctx->cam.position, ctx->cam.up);
	f32 aspect_ratio = (f32)state.window.width/(f32)state.window.height;
	Mat4 proj = create_perspective(aspect_ratio, 90.0f, 0.1f, 100.0f);
    ctx->lookat = lookat;
    ctx->proj = proj;
    ctx->ortho = create_ortographic(aspect_ratio, 0.01f, 10.0f);
    
    glEnable(GL_DEPTH_TEST);
	Line ray_line = {};
	
	glfwSwapInterval(1);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(state.window.ptr, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
	f64 xmouse, ymouse;
	f64 xmouseold, ymouseold;
	glfwGetCursorPos(state.window.ptr, &xmouse, &ymouse);
	while(!glfwWindowShouldClose(state.window.ptr))
	{
		glfwPollEvents();
        state.timer.frame_start = glfwGetTime();
		
        // TODO(mateusz): REMOVE IT! Read note in render_flush()
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		xmouseold = xmouse;
		ymouseold = ymouse;
		glfwGetCursorPos(state.window.ptr, &xmouse, &ymouse);
        if(!state.in_editor)
            camera_mouse_moved(&ctx->cam, xmouse - xmouseold, ymouse - ymouseold);
		
		buttons_update(state.kbuttons, ARRAY_LEN(state.kbuttons));
		buttons_update(state.mbuttons, ARRAY_LEN(state.mbuttons));
		
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if(state.in_editor)
        {
            ImGui::Begin("Editor");
            
            ImGui::Checkbox("Use mapped normals", &state.use_mapped_normals);
            ImGui::Checkbox("Shade with normal map", &state.show_normal_map);
            ImGui::SliderFloat("backpack.position.x", &monkey.size.x, 0.0f, 1.0f);
            ImGui::SliderFloat("backpack.position.y", &monkey.size.y, 0.0f, 1.0f);
            ImGui::SliderFloat("backpack.position.z", &monkey.size.z, 0.0f, 1.0f);
            
            ImGui::SameLine();
            
            ImGui::End();
        }
        
		lookat = look_at(add(ctx->cam.front, ctx->cam.position), ctx->cam.position, ctx->cam.up);
        ctx->lookat = lookat;
		if(state.kbuttons[GLFW_KEY_P].pressed)
        {
            glfwSetInputMode(state.window.ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        
        if(state.kbuttons[GLFW_KEY_E].pressed)
        {
            state.in_editor = !state.in_editor;
            
            if(state.in_editor) {
                glfwSetInputMode(state.window.ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(state.window.ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        
        if(state.kbuttons[GLFW_KEY_R].pressed)
        {
            // TODO(mateusz): Shader reloading
            glDeleteProgram(main_program);
            //basic_program = basic_program_build();
        }
        
        if(state.kbuttons[GLFW_KEY_H].pressed)
        {
            state.draw_hitboxes = !state.draw_hitboxes;
        }
        
		if(state.kbuttons[GLFW_KEY_M].pressed && (monkey_model.flags & MODEL_FLAGS_GOURAUD_SHADED))
		{
			model_mesh_normals_shade(&monkey_model);
		}
		
		if(state.kbuttons[GLFW_KEY_N].pressed && monkey_model.flags & MODEL_FLAGS_MESH_NORMALS_SHADED)
		{
			model_gouraud_shade(&monkey_model);
		}
		
		float movement_scalar = 0.1f;
		if(state.kbuttons[GLFW_KEY_W].down) {
			ctx->cam.position = add(ctx->cam.position, scale(ctx->cam.front, movement_scalar));
		} if(state.kbuttons[GLFW_KEY_S].down) {
			ctx->cam.position = sub(ctx->cam.position, scale(ctx->cam.front, movement_scalar));
		} if(state.kbuttons[GLFW_KEY_D].down) {
			ctx->cam.position = add(ctx->cam.position, scale(ctx->cam.right, movement_scalar));
		} if(state.kbuttons[GLFW_KEY_A].down) {
			ctx->cam.position = sub(ctx->cam.position, scale(ctx->cam.right, movement_scalar));
		}
		
		if(state.kbuttons[GLFW_KEY_SPACE].pressed)
		{
			ray_line = line_from_direction(ctx->cam.position, ctx->cam.front, 10.0f);
			
			u64 start = rdtsc();
			bool entity_hit = ray_intersect_entity(ctx->cam.position, ctx->cam.front, &monkey);
			u64 end = rdtsc();
			
			u64 hit_clocks = end - start;
			printf("EntityHit = %d | %lu clocks/call\n", entity_hit, hit_clocks);
		}
		
        monkey.rotate = create_qrot(to_radians(glfwGetTime() * 14.0f) * 8.0f, Vec3(1.0f, 0.0f, 0.0f));
        backpack.rotate = create_qrot(to_radians(glfwGetTime() * 8.0f) * 13.0f, Vec3(1.0f, 0.4f, 0.2f));
        
        render_push_line(rqueue, ray_line);
        
        render_push_skybox(rqueue, skybox);
        
		glUseProgram(main_program);
        
        opengl_set_uniform(main_program, "view", lookat);
        opengl_set_uniform(main_program, "proj", proj);
		
        opengl_set_uniform(main_program, "view_pos", ctx->cam.position);
        
        opengl_set_uniform(main_program, "light_pos", ctx->cam.position);
        opengl_set_uniform(main_program, "spotlight.direction", ctx->cam.front);
        opengl_set_uniform(main_program, "spotlight.cutoff", cosf(to_radians(22.5f)));
        opengl_set_uniform(main_program, "spotlight.outer_cutoff", cosf(to_radians(27.5f)));
        opengl_set_uniform(main_program, "spotlight.ambient_component", Vec3(0.1f, 0.1f, 0.1f));
        opengl_set_uniform(main_program, "spotlight.diffuse_component", Vec3(1.8f, 1.8f, 1.8f));
        opengl_set_uniform(main_program, "spotlight.specular_component", Vec3(1.0f, 1.0f, 1.0f));
        opengl_set_uniform(main_program, "spotlight.atten_const", 1.0f);
        opengl_set_uniform(main_program, "spotlight.atten_linear", 0.09f);
        opengl_set_uniform(main_program, "spotlight.atten_quad", 0.032f);
        
        opengl_set_uniform(main_program, "direct_light.direction", Vec3(0.0f, -1.0f, 0.0f));
        opengl_set_uniform(main_program, "direct_light.ambient_component", Vec3(0.05f, 0.05f, 0.05f));
        opengl_set_uniform(main_program, "direct_light.diffuse_component", Vec3(0.2f, 0.2f, 0.2f));
        opengl_set_uniform(main_program, "direct_light.specular_component", Vec3(0.4f, 0.4f, 0.4f));
		
        glUniform1i(glGetUniformLocation(main_program, "show_normal_map"), state.show_normal_map);
        glUniform1i(glGetUniformLocation(main_program, "use_mapped_normals"), state.use_mapped_normals);
        
        entity_draw(backpack, main_program);
        entity_draw(crysis_guy, main_program);
        entity_draw(cyborg, main_program);
        
        glUseProgram(simple_program);
        
        opengl_set_uniform(simple_program, "view", lookat);
        opengl_set_uniform(simple_program, "proj", proj);
		
        opengl_set_uniform(simple_program, "view_pos", ctx->cam.position);
        
        opengl_set_uniform(simple_program, "light_pos", ctx->cam.position);
        opengl_set_uniform(simple_program, "spotlight.direction", ctx->cam.front);
        opengl_set_uniform(simple_program, "spotlight.cutoff", cosf(to_radians(22.5f)));
        opengl_set_uniform(simple_program, "spotlight.outer_cutoff", cosf(to_radians(27.5f)));
        opengl_set_uniform(simple_program, "spotlight.ambient_component", Vec3(0.1f, 0.1f, 0.1f));
        opengl_set_uniform(simple_program, "spotlight.diffuse_component", Vec3(1.8f, 1.8f, 1.8f));
        opengl_set_uniform(simple_program, "spotlight.specular_component", Vec3(1.0f, 1.0f, 1.0f));
        opengl_set_uniform(simple_program, "spotlight.atten_const", 1.0f);
        opengl_set_uniform(simple_program, "spotlight.atten_linear", 0.09f);
        opengl_set_uniform(simple_program, "spotlight.atten_quad", 0.032f);
        
        opengl_set_uniform(simple_program, "direct_light.direction", Vec3(0.0f, -1.0f, 0.0f));
        opengl_set_uniform(simple_program, "direct_light.ambient_component", Vec3(0.05f, 0.05f, 0.05f));
        opengl_set_uniform(simple_program, "direct_light.diffuse_component", Vec3(0.2f, 0.2f, 0.2f));
        opengl_set_uniform(simple_program, "direct_light.specular_component", Vec3(0.4f, 0.4f, 0.4f));
        
        entity_draw(monkey, simple_program);
		entity_draw(floor, simple_program);
		
        if(state.draw_hitboxes)
        {
            render_push_hitbox(rqueue, monkey);
            render_push_hitbox(rqueue, backpack);
            render_push_hitbox(rqueue, crysis_guy);
            render_push_hitbox(rqueue, cyborg);
        }
        render_push_ui(rqueue, crosshair);
		
        render_flush(rqueue, ctx);
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
		glfwSwapBuffers(state.window.ptr);
        
        state.timer.frame_end = glfwGetTime();
        state.timer.since_last_second += state.timer.frame_end - state.timer.frame_start;
        state.timer.frames++;
        if(state.timer.since_last_second >= 1.0)
        {
            window_debug_title(&state.window, state.timer);
            state.timer.since_last_second -= 1.0;
            state.timer.frames = 0;
        }
	}
	
    render_destory_queue(rqueue);
    
    model_destory(monkey_model);
    model_destory(backpack_model);
    model_destory(crysis_model);
    model_destory(cyborg_model);
    model_destory(floor_model);
    glfwTerminate();
	
	return 0;
}