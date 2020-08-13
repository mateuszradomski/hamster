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

	Mesh *basic_mesh = mesh_create_basic();
	Mesh *obj_mesh = mesh_create_from_obj("data/model.obj");
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

	model = rot_around_vec(model, to_radians(22.5f), Vec3(0.0f, 1.0f, 0.0f));

	glUseProgram(basic_program);
	glBindVertexArray(basic_mesh->vao);

	glEnable(GL_DEPTH_TEST);

	while(!glfwWindowShouldClose(state.window.ptr))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// glBindVertexArray(basic_mesh->vao);
		// glDrawArrays(GL_TRIANGLES, 0, 3);

		GLint uniform_location = glGetUniformLocation(basic_program, "view");
		glUniformMatrix4fv(uniform_location, 1, GL_FALSE, lookat.a1d);
		uniform_location = glGetUniformLocation(basic_program, "proj");
		glUniformMatrix4fv(uniform_location, 1, GL_FALSE, proj.a1d);
		uniform_location = glGetUniformLocation(basic_program, "model");
		glUniformMatrix4fv(uniform_location, 1, GL_FALSE, model.a1d);

		uniform_location = glGetUniformLocation(basic_program, "light.position");
		glUniform3fv(uniform_location, 1, camera.position.m);
		uniform_location = glGetUniformLocation(basic_program, "light.direction");
		glUniform3fv(uniform_location, 1, camera.front.m);
		uniform_location = glGetUniformLocation(basic_program, "light.cutoff");
		glUniform1f(uniform_location, cosf(to_radians(12.5f)));
		uniform_location = glGetUniformLocation(basic_program, "light.outer_cutoff");
		glUniform1f(uniform_location, cosf(to_radians(15.5f)));

		glBindVertexArray(obj_mesh->vao);
		glDrawElements(GL_TRIANGLES, 2904, GL_UNSIGNED_INT, NULL);
		assert(glGetError() == GL_NO_ERROR);

		glfwSwapBuffers(state.window.ptr);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
