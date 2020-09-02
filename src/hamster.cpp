#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

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

struct Window
{
	GLFWwindow *ptr;
	int width;
	int height;
	
	Window() :
	ptr(NULL), width(1280), height(720)
	{ }
};

struct Button
{
	u8 down : 1;
	u8 last : 1;
	u8 pressed : 1;
};

struct ProgramState
{
	Window window;
	Timer timer;
    
    bool draw_hitboxes;
    bool in_editor;
    bool show_normal_map;
    bool use_mapped_normals;
	Button kbuttons[GLFW_KEY_LAST];
	Button mbuttons[GLFW_MOUSE_BUTTON_LAST];
};

void
opengl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
					  GLsizei length, const GLchar *message, const void *userParam)
{
	(void)source;
	(void)type;
	(void)id;
	(void)severity;
	(void)length;
	(void)userParam;
	
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
	
	//glfwSetInputMode(window.ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
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
	(void)mods; // NOTE: I guess we would use it somewhere??
	(void)scancode; // That is unused.
	
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
	(void)mods; // NOTE: I guess we would use it somewhere??
	
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
	Cubemap skybox = cubemap_create_skybox();
    GLuint skybox_program = program_create_from_file(SKYBOX_VERTEX_FILENAME, SKYBOX_FRAG_FILENAME);
    
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
	
	Camera camera = {};
	camera.position = Vec3(0.0f, 0.0f, 3.0f);
	camera.yaw = asinf(-1.0f); // Where we look
	camera_calculate_vectors(&camera);
	
	// const Vec3 world_up = Vec3(0.0f, 1.0f, 0.0f); // Don't repeat yourself
	Mat4 lookat = look_at(camera.front, camera.position, camera.up);
	f32 aspect_ratio = (f32)state.window.width/(f32)state.window.height;
	Mat4 proj = create_perspective(aspect_ratio, 90.0f, 0.1f, 100.0f);
	Mat4 model = Mat4(1.0f);
	
    BasicShaderProgram basic_program = basic_program_build();
	GLuint line_program = program_create_from_file(MAIN_VERTEX_FILENAME, LINE_FRAG_FILENAME);
    GLuint simple_program = program_create_from_file(SIMPLE_VERTEX_FILENAME, SIMPLE_FRAG_FILENAME);
    
    glEnable(GL_DEPTH_TEST);
	Line ray_line = {};
	GLuint line_vao, line_vbo;
	glGenVertexArrays(1, &line_vao);
	glBindVertexArray(line_vao);
	glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
	
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
        state.timer.frame_start = glfwGetTime();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		xmouseold = xmouse;
		ymouseold = ymouse;
		glfwGetCursorPos(state.window.ptr, &xmouse, &ymouse);
        if(!state.in_editor)
            camera_mouse_moved(&camera, xmouse - xmouseold, ymouse - ymouseold);
		
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
        
		lookat = look_at(add(camera.front, camera.position), camera.position, camera.up);
		model = Mat4(1.0f);
		if(state.kbuttons[GLFW_KEY_P].pressed)
        {
            glfwSetInputMode(state.window.ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        
        if(state.kbuttons[GLFW_KEY_E].pressed)
        {
            state.in_editor = !state.in_editor;
        }
        
        if(state.kbuttons[GLFW_KEY_R].pressed)
        {
            glDeleteProgram(basic_program.id);
            basic_program = basic_program_build();
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
			camera.position = add(camera.position, scale(camera.front, movement_scalar));
		} if(state.kbuttons[GLFW_KEY_S].down) {
			camera.position = sub(camera.position, scale(camera.front, movement_scalar));
		} if(state.kbuttons[GLFW_KEY_D].down) {
			camera.position = add(camera.position, scale(camera.right, movement_scalar));
		} if(state.kbuttons[GLFW_KEY_A].down) {
			camera.position = sub(camera.position, scale(camera.right, movement_scalar));
		}
		
		if(state.kbuttons[GLFW_KEY_SPACE].pressed)
		{
			ray_line = line_from_direction(camera.position, camera.front, 10.0f);
			
			u64 start = rdtsc();
			bool entity_hit = ray_intersect_entity(camera.position, camera.front, &monkey);
			u64 end = rdtsc();
			
			u64 hit_clocks = end - start;
			printf("EntityHit = %d | %lu clocks/call\n", entity_hit, hit_clocks);
		}
		
        //monkey.rotate = create_qrot(to_radians(glfwGetTime() * 14.0f) * 8.0f, Vec3(1.0f, 0.0f, 0.0f));
        //backpack.rotate = create_qrot(to_radians(glfwGetTime() * 8.0f) * 13.0f, Vec3(1.0f, 0.4f, 0.2f));
        
        glBindVertexArray(line_vao);
		glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(ray_line), &ray_line, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glUseProgram(line_program);
		opengl_set_uniform(line_program, "view", lookat);
		opengl_set_uniform(line_program, "proj", proj);
		opengl_set_uniform(line_program, "model", model);
		glDrawArrays(GL_LINES, 0, 2);
		glUseProgram(0);
		
        glUseProgram(skybox_program);
        Mat4 view_no_translation = lookat;
        view_no_translation.columns[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        opengl_set_uniform(skybox_program, "view", view_no_translation);
        opengl_set_uniform(skybox_program, "proj", proj);
        
        cubemap_draw_skybox(skybox);
        
        glUseProgram(0);
        
		glUseProgram(basic_program.id);
		glUniformMatrix4fv(basic_program.view, 1, GL_FALSE, lookat.a1d);
		glUniformMatrix4fv(basic_program.proj, 1, GL_FALSE, proj.a1d);
		glUniformMatrix4fv(basic_program.model, 1, GL_FALSE, model.a1d);
		
		glUniform3fv(basic_program.view_pos, 1, camera.position.m);
		
		glUniform3fv(basic_program.spotlight_position, 1, camera.position.m);
		glUniform3fv(basic_program.spotlight_direction, 1, camera.front.m);
		glUniform1f(basic_program.spotlight_cutoff, cosf(to_radians(22.5f)));
		glUniform1f(basic_program.spotlight_outer_cutoff, cosf(to_radians(27.5f)));
		glUniform3fv(basic_program.spotlight_ambient_component, 1, Vec3(0.1f, 0.1f, 0.1f).m);
		glUniform3fv(basic_program.spotlight_diffuse_component, 1, Vec3(1.8f, 1.8f,1.8f).m);
		glUniform3fv(basic_program.spotlight_specular_component, 1, Vec3(1.0f, 1.0f, 1.0f).m);
		glUniform1f(basic_program.spotlight_atten_const, 1.0f);
		glUniform1f(basic_program.spotlight_atten_linear, 0.09f);
		glUniform1f(basic_program.spotlight_atten_quad, 0.032f);
		
		glUniform3fv(basic_program.direct_light_direction, 1, Vec3(0.0f, -1.0f, 0.0f).m);
		glUniform3fv(basic_program.direct_light_ambient_component, 1, Vec3(0.05f, 0.05f, 0.05f).m);
		glUniform3fv(basic_program.direct_light_diffuse_component, 1, Vec3(0.2f, 0.2f, 0.2f).m);
		glUniform3fv(basic_program.direct_light_specular_component, 1, Vec3(0.4f, 0.4f, 0.4f).m);
		
        glUniform1i(glGetUniformLocation(basic_program.id, "show_normal_map"), state.show_normal_map);
        glUniform1i(glGetUniformLocation(basic_program.id, "use_mapped_normals"), state.use_mapped_normals);
        
        entity_draw(backpack, basic_program);
        entity_draw(crysis_guy, basic_program);
        entity_draw(cyborg, basic_program);
        
        glUseProgram(simple_program);
        
        opengl_set_uniform(simple_program, "view", lookat);
        opengl_set_uniform(simple_program, "proj", proj);
		
        opengl_set_uniform(simple_program, "view_pos", camera.position);
        
        opengl_set_uniform(simple_program, "light_pos", camera.position);
        opengl_set_uniform(simple_program, "spotlight.direction", camera.front);
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
        
        GLuint id = basic_program.id;
        GLuint model_id = basic_program.model;
        GLuint ambient = basic_program.material_ambient_component;
        GLuint diffuse = basic_program.material_diffuse_component;
        GLuint specular = basic_program.material_specular_component;
        GLuint specular_exp = basic_program.material_specular_exponent;
        
        basic_program.id = simple_program;
        basic_program.model = glGetUniformLocation(simple_program, "model");
        basic_program.material_ambient_component = glGetUniformLocation(simple_program, "material.ambient_component");
        basic_program.material_diffuse_component = glGetUniformLocation(simple_program, "material.diffuse_component");
        basic_program.material_specular_component = glGetUniformLocation(simple_program, "material.specular_component");
        basic_program.material_specular_exponent = glGetUniformLocation(simple_program, "material.specular_exponent");
        
        
        entity_draw(monkey, basic_program);
		entity_draw(floor, basic_program);
		
        basic_program.id = id;
        basic_program.model = model_id;
        basic_program.material_ambient_component = ambient;
        basic_program.material_diffuse_component = diffuse;
        basic_program.material_specular_component = specular;
        basic_program.material_specular_exponent = specular_exp;
        
        if(state.draw_hitboxes)
        {
            glUseProgram(line_program);
            entity_draw_hitbox(monkey, line_program);
            entity_draw_hitbox(backpack, line_program);
            entity_draw_hitbox(crysis_guy, line_program);
            entity_draw_hitbox(cyborg, line_program);
        }
		
		ui_element_draw(crosshair);
		
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
		glfwSwapBuffers(state.window.ptr);
		glfwPollEvents();
        
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
	
    model_destory(monkey_model);
    model_destory(backpack_model);
    model_destory(crysis_model);
    model_destory(cyborg_model);
    model_destory(floor_model);
    glfwTerminate();
	
	return 0;
}
