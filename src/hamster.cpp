#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "hamster_math.cpp"
#include "hamster_util.cpp"
#include "hamster_graphics.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "includes/stb_image.h"

struct Window
{
	GLFWwindow *ptr;
	int width;
	int height;
	
	Window() :
	ptr(NULL), width(800), height(600)
	{ }
};

struct ProgramState
{
	Window window;
};

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

int main()
{
	ProgramState state = {};
	state.window = create_opengl_window();
	
	Model *basic_model = model_create_basic();
	Model *obj_model = model_create_from_obj("data/model.obj");
	Model *floor_model = model_create_debug_floor();
	GLuint basic_program = program_create_basic();
	
	Camera camera = {};
	camera.position = Vec3(0.0f, 0.0f, 3.0f);
	camera.front = negate(noz(camera.position)); // Where we look
	camera_calculate_vectors(&camera);
	
	// const Vec3 world_up = Vec3(0.0f, 1.0f, 0.0f); // Don't repeat yourself
	Mat4 lookat = look_at(camera.front, camera.position, camera.up);
	f32 aspect_ratio = (f32)state.window.width/(f32)state.window.height;
	Mat4 proj = create_perspective(aspect_ratio, 90.0f, 0.1f, 10.0f);
	Mat4 model = Mat4(1.0f);
	
	i32 wimg, himg, channelnr = 0;
	u8 *img_pixels = stbi_load("data/wood.png", &wimg, &himg, &channelnr, 0);
	assert(img_pixels);
	assert(channelnr == 3);
	
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wimg, himg, 0, GL_RGB, GL_UNSIGNED_BYTE, img_pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	free(img_pixels);
	
	glUseProgram(basic_program);
	glBindVertexArray(basic_model->vao);
	
	glEnable(GL_DEPTH_TEST);
	
	f64 xmouse, ymouse;
	f64 xmouseold, ymouseold;
	glfwGetCursorPos(state.window.ptr, &xmouse, &ymouse);
	while(!glfwWindowShouldClose(state.window.ptr))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// glBindVertexArray(basic_model->vao);
		// glDrawArrays(GL_TRIANGLES, 0, 3);
		
		xmouseold = xmouse;
		ymouseold = ymouse;
		glfwGetCursorPos(state.window.ptr, &xmouse, &ymouse);
		camera_mouse_moved(&camera, xmouse - xmouseold, ymouse - ymouseold);
		
		lookat = look_at(add(camera.front, camera.position), camera.position, camera.up);
		model = rot_around_vec(Mat4(1.0), glfwGetTime(), Vec3(0.0f, 1.0f, 0.0f));
		
		if(glfwGetKey(state.window.ptr, GLFW_KEY_M) &&
		   (obj_model->state & MODEL_STATE_GOURAUD_SHADED))
		{
			model_mesh_normals_shade(obj_model);
		}
		
		if(glfwGetKey(state.window.ptr, GLFW_KEY_N) && obj_model->state & MODEL_STATE_MESH_NORMALS_SHADED)
		{
			model_gouraud_shade(obj_model);
		}
		
		if(glfwGetKey(state.window.ptr, GLFW_KEY_W))
		{
			camera.position.z -= 0.01f;
		}
		if(glfwGetKey(state.window.ptr, GLFW_KEY_S))
		{
			camera.position.z += 0.01f;
		}
		
		opengl_set_uniform(basic_program, "view", lookat);
		opengl_set_uniform(basic_program, "proj", proj);
		opengl_set_uniform(basic_program, "model", model);
		
		opengl_set_uniform(basic_program, "view_pos", camera.position);
		
		opengl_set_uniform(basic_program, "spotlight.position", camera.position);
		opengl_set_uniform(basic_program, "spotlight.direction", camera.front);
		opengl_set_uniform(basic_program, "spotlight.cutoff", cosf(to_radians(12.5f)));
		opengl_set_uniform(basic_program, "spotlight.outer_cutoff", cosf(to_radians(16.5f)));
		opengl_set_uniform(basic_program, "spotlight.ambient_component", Vec3(0.1f, 0.1f, 0.1f));
		opengl_set_uniform(basic_program, "spotlight.diffuse_component", Vec3(0.8f, 0.8f, 0.8f));
		opengl_set_uniform(basic_program, "spotlight.specular_component", Vec3(1.0f, 1.0f, 1.0f));
		opengl_set_uniform(basic_program, "spotlight.atten_const", 1.0f);
		opengl_set_uniform(basic_program, "spotlight.atten_linear", 0.09f);
		opengl_set_uniform(basic_program, "spotlight.atten_quad", 0.032f);
		
		opengl_set_uniform(basic_program, "direct_light.direction", Vec3(0.0f, -1.0f, 0.0f));
		opengl_set_uniform(basic_program, "direct_light.ambient_component", Vec3(0.05f, 0.05f, 0.05f));
		opengl_set_uniform(basic_program, "direct_light.diffuse_component", Vec3(0.2f, 0.2f, 0.2f));
		opengl_set_uniform(basic_program, "direct_light.specular_component", Vec3(0.4f, 0.4f, 0.4f));
		
		glBindVertexArray(obj_model->vao);
		glDrawElements(GL_TRIANGLES, obj_model->indices.length, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
		
		model = scale(Mat4(1.0f), Vec3(3.0f, 1.0f, 3.0f));
		model = translate(model, Vec3(0.0f, -2.0f, -3.0f));
		opengl_set_uniform(basic_program, "model", model);
		
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(floor_model->vao);
		glDrawElements(GL_TRIANGLES, floor_model->indices.length, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
		assert(glGetError() == GL_NO_ERROR);
		
		glfwSwapBuffers(state.window.ptr);
		glfwPollEvents();
	}
	
	glfwTerminate();
	
	return 0;
}
