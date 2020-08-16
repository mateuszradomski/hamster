#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "hamster_math.cpp"
#include "hamster_util.cpp"
#include "hamster_graphics.cpp"

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
	
	glUseProgram(basic_program);
	glBindVertexArray(basic_model->vao);
	
	glEnable(GL_DEPTH_TEST);
	
	while(!glfwWindowShouldClose(state.window.ptr))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// glBindVertexArray(basic_model->vao);
		// glDrawArrays(GL_TRIANGLES, 0, 3);
		
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
		
		opengl_set_uniform(basic_program, "view", lookat);
		opengl_set_uniform(basic_program, "proj", proj);
		opengl_set_uniform(basic_program, "model", model);
		
		opengl_set_uniform(basic_program, "light.position", camera.position);
		opengl_set_uniform(basic_program, "light.direction", camera.front);
		opengl_set_uniform(basic_program, "light.cutoff", cosf(to_radians(12.5f)));
		opengl_set_uniform(basic_program, "light.outer_cutoff", cosf(to_radians(16.5f)));
		
		glBindVertexArray(obj_model->vao);
		glDrawElements(GL_TRIANGLES, obj_model->indices.length, GL_UNSIGNED_INT, NULL);
		assert(glGetError() == GL_NO_ERROR);
		
		glfwSwapBuffers(state.window.ptr);
		glfwPollEvents();
	}
	
	glfwTerminate();
	
	return 0;
}
