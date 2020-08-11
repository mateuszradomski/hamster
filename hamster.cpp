#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "hamster_util.cpp"
#include "hamster_graphics.cpp"

const int width = 800;
const int height = 600;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(width, height, "hamster", NULL, NULL);
	glfwMakeContextCurrent(window);

	glewInit();

	Mesh *basic_mesh = mesh_create_basic();
	GLuint basic_program = program_create_basic();

	obj_load("model.obj");

	glUseProgram(basic_program);
	glBindVertexArray(basic_mesh->vao);

	while(!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}