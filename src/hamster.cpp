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

#include "hamster_math.h"
#include "hamster_util.h"
#include "hamster_graphics.h"
#include "hamster_scene.h"
#include "hamster_render.h"
#include "hamster.h"

#include "hamster_math.cpp"
#include "hamster_util.cpp"
#include "hamster_graphics.cpp"
#include "hamster_scene.cpp"
#include "hamster_render.cpp"

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

static void
set_vsync(bool on)
{
    u32 value = on ? 1 : 0;
    glfwSwapInterval(value);
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
    
    NOT_USED(tpoints);
    
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
	
    RenderContext ctx_ = {};
    RenderContext *ctx = &ctx_;
    render_load_programs(ctx);
    
	ctx->cam.position = Vec3(0.0f, 0.0f, 3.0f);
	ctx->cam.yaw = asinf(-1.0f); // Where we look
	camera_calculate_vectors(&ctx->cam);
	
    ctx->spot = spotlight_at_camera(ctx->cam);
    ctx->spot.cutoff = 20.0f;
    ctx->spot.outer_cutoff = 25.0f;
    
    ctx->spot.ambient_part = Vec3(0.1f, 0.1f, 0.1f);
    ctx->spot.diffuse_part = Vec3(0.8f, 0.8f, 0.8f);
    ctx->spot.specular_part = Vec3(1.0f, 1.0f, 1.0f);
    
    ctx->spot.atten_const = 1.0f;
    ctx->spot.atten_linear = 0.09f;
    ctx->spot.atten_quad = 0.032f;
    
    ctx->sun = create_sun(Vec3(0.0f, -1.0f, 0.0f), 0.1f, 0.4f, 0.7f);
    
    ctx->point_light.ambient_part = Vec3(0.2f, 0.2f, 0.2f);
    ctx->point_light.diffuse_part = Vec3(0.9f, 0.9f, 0.9f);
    ctx->point_light.specular_part = Vec3(1.0f, 1.0f, 1.0f);
    
    ctx->point_light.atten_const = 1.0;
    ctx->point_light.atten_linear = 0.022;
    ctx->point_light.atten_quad = 0.0019;
    
    RenderQueue rqueue_ = render_create_queue();
    RenderQueue *rqueue = &rqueue_;
    
    Mat4 lookat = look_at(ctx->cam.front, ctx->cam.position, ctx->cam.up);
	f32 aspect_ratio = (f32)state.window.width/(f32)state.window.height;
	Mat4 proj = create_perspective(aspect_ratio, 90.0f, 0.1f, 100.0f);
    ctx->lookat = lookat;
    ctx->proj = proj;
    ctx->ortho = create_ortographic(aspect_ratio, 0.01f, 10.0f);
    
    glGenFramebuffers(1, &ctx->hdr_fbo);
    
    glGenTextures(1, &ctx->color_buffer);
    glBindTexture(GL_TEXTURE_2D, ctx->color_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (f32)state.window.width, (f32)state.window.height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glGenRenderbuffers(1, &ctx->rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, ctx->rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (f32)state.window.width, (f32)state.window.height);
    
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->hdr_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->color_buffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ctx->rbo_depth);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glEnable(GL_DEPTH_TEST);
	Line ray_line = {};
    Line to_point_light = {};
	
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(state.window.ptr, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    set_vsync(true);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    f64 xmouse, ymouse;
	f64 xmouseold, ymouseold;
	glfwGetCursorPos(state.window.ptr, &xmouse, &ymouse);
	while(!glfwWindowShouldClose(state.window.ptr))
	{
		glfwPollEvents();
        state.timer.frame_start = glfwGetTime();
		
		xmouseold = xmouse;
		ymouseold = ymouse;
		glfwGetCursorPos(state.window.ptr, &xmouse, &ymouse);
        if(!state.in_editor)
            camera_mouse_moved(&ctx->cam, xmouse - xmouseold, ymouse - ymouseold);
        
        ctx->spot.position = ctx->cam.position;
        ctx->spot.direction = ctx->cam.front;
		
		buttons_update(state.kbuttons, ARRAY_LEN(state.kbuttons));
		buttons_update(state.mbuttons, ARRAY_LEN(state.mbuttons));
		
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
        
        if(state.kbuttons[GLFW_KEY_H].pressed)
        {
            ctx->draw_hitboxes = !ctx->draw_hitboxes;
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
        
        if(state.in_editor)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            ImGui::Begin("Editor");
            
            ImGui::Checkbox("Use mapped normals", &ctx->use_mapped_normals);
            ImGui::Checkbox("Shade with normal map", &ctx->show_normal_map);
            if(ImGui::TreeNode("Spotlight"))
            {
                ImGui::SliderFloat("spot.cutoff", &ctx->spot.cutoff, 0.0f, 90.0f);
                ImGui::SliderFloat("spot.outer_cutoff", &ctx->spot.outer_cutoff, 0.0f, 90.0f);
                ImGui::SliderFloat("spot.ambient", &ctx->spot.ambient_part.x, 0.0f, 1.0f);
                ctx->spot.ambient_part.y = ctx->spot.ambient_part.z = ctx->spot.ambient_part.x;
                ImGui::SliderFloat("spot.diffuse", &ctx->spot.diffuse_part.x, 0.0f, 1.0f);
                ctx->spot.diffuse_part.y = ctx->spot.diffuse_part.z = ctx->spot.diffuse_part.x;
                ImGui::SliderFloat("spot.specular", &ctx->spot.specular_part.x, 0.0f, 1.0f);
                ctx->spot.specular_part.y = ctx->spot.specular_part.z = ctx->spot.specular_part.x;
                ImGui::SliderFloat("spot.atten_const", &ctx->spot.atten_const, 0.0f, 1.0f);
                ImGui::SliderFloat("spot.atten_linear", &ctx->spot.atten_linear, 0.0f, 1.0f);
                ImGui::SliderFloat("spot.atten_quad", &ctx->spot.atten_quad, 0.0f, 1.0f);
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Pointlight"))
            {
                ImGui::SliderFloat3("point.position", ctx->point_light.position.m, -10.0f, 10.0f);
                
                ImGui::SliderFloat("point.ambient", &ctx->point_light.ambient_part.x, 0.0f, 1.0f);
                ctx->point_light.ambient_part.y = ctx->point_light.ambient_part.z = ctx->point_light.ambient_part.x;
                ImGui::SliderFloat("point.diffuse", &ctx->point_light.diffuse_part.x, 0.0f, 1.0f);
                ctx->point_light.diffuse_part.y = ctx->point_light.diffuse_part.z = ctx->point_light.diffuse_part.x;
                ImGui::SliderFloat("point.specular", &ctx->point_light.specular_part.x, 0.0f, 1.0f);
                ctx->point_light.specular_part.y = ctx->point_light.specular_part.z = ctx->point_light.specular_part.x;
                ImGui::SliderFloat("point.atten_const", &ctx->point_light.atten_const, 0.0f, 1.0f);
                ImGui::SliderFloat("point.atten_linear", &ctx->point_light.atten_linear, 0.0f, 1.0f);
                ImGui::SliderFloat("point.atten_quad", &ctx->point_light.atten_quad, 0.0f, 1.0f);
                ImGui::TreePop();
            }
            
            if(ImGui::TreeNode("Directional light / Sun"))
            {
                ImGui::SliderFloat3("sun.direction", ctx->sun.direction.m, -1.0f, 1.0f);
                
                ImGui::SliderFloat("sun.ambient", &ctx->sun.ambient_part.x, 0.0f, 1.0f);
                ctx->sun.ambient_part.y = ctx->sun.ambient_part.z = ctx->sun.ambient_part.x;
                ImGui::SliderFloat("sun.diffuse", &ctx->sun.diffuse_part.x, 0.0f, 1.0f);
                ctx->sun.diffuse_part.y = ctx->sun.diffuse_part.z = ctx->sun.diffuse_part.x;
                ImGui::SliderFloat("sun.specular", &ctx->sun.specular_part.x, 0.0f, 1.0f);
                
                ImGui::TreePop();
            }
            
            ImGui::SameLine();
            
            ImGui::End();
        }
        
        monkey.rotate = create_qrot(to_radians(glfwGetTime() * 14.0f) * 8.0f, Vec3(1.0f, 0.0f, 0.0f));
        backpack.rotate = create_qrot(to_radians(glfwGetTime() * 8.0f) * 13.0f, Vec3(1.0f, 0.4f, 0.2f));
        
        ctx->point_light.position = Vec3(5.0f * cosf(glfwGetTime()), 0.0f, 5.0f * sinf(glfwGetTime()));
        
        to_point_light.point0 = sub(ctx->cam.position, Vec3(0.1f, 0.1f, 0.1f));
        to_point_light.point1 = ctx->point_light.position;
        
        render_push_line(rqueue, ray_line);
        render_push_line(rqueue, to_point_light);
        
        render_push_skybox(rqueue, skybox);
        
        render_push_model_newest(rqueue, backpack);
        render_push_model_newest(rqueue, crysis_guy);
        render_push_model_newest(rqueue, cyborg);
        
        render_push_model(rqueue, monkey);
        render_push_model(rqueue, floor);
        
        if(ctx->draw_hitboxes)
        {
            render_push_hitbox(rqueue, monkey);
            render_push_hitbox(rqueue, backpack);
            render_push_hitbox(rqueue, crysis_guy);
            render_push_hitbox(rqueue, cyborg);
        }
        render_push_ui(rqueue, crosshair);
		
        render_flush(rqueue, ctx);
        
        if(state.in_editor)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        
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