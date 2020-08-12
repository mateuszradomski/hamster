#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "hamster_math.cpp"
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
	Mesh *obj_mesh = mesh_create_from_obj("data/model.obj");
	GLuint basic_program = program_create_basic();

	Camera camera = {};
	camera.position = Vec3(0.0f, 0.0f, 3.0f);
	camera.front = Vec3(0.0f, 0.0f, 1.0f); // Where we look
	camera_calculate_vectors(&camera);

	// const Vec3 world_up = Vec3(0.0f, 1.0f, 0.0f); // Don't repeat yourself
	Mat4 lookat = look_at(camera.front, camera.position, camera.up);
	Mat4 proj = create_perspective((f32)width/(f32)height, 90.0f, 0.1f, 10.0f);
	Mat4 model = Mat4(1.0f);

	glUseProgram(basic_program);
	glBindVertexArray(basic_mesh->vao);

	while(!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		// glBindVertexArray(basic_mesh->vao);
		// glDrawArrays(GL_TRIANGLES, 0, 3);

		GLint uniform_location = glGetUniformLocation(basic_program, "view");
		glUniformMatrix4fv(uniform_location, 1, GL_FALSE, lookat.a1d);
		uniform_location = glGetUniformLocation(basic_program, "proj");
		glUniformMatrix4fv(uniform_location, 1, GL_FALSE, proj.a1d);
		uniform_location = glGetUniformLocation(basic_program, "model");
		glUniformMatrix4fv(uniform_location, 1, GL_FALSE, model.a1d);

		glBindVertexArray(obj_mesh->vao);
		glDrawElements(GL_TRIANGLES, 2904, GL_UNSIGNED_INT, NULL);
		assert(glGetError() == GL_NO_ERROR);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}