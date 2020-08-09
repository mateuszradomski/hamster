#include <cstdio>

#include <GLFW/glfw3.h>

const int width = 800;
const int height = 600;

int main()
{
	glfwInit();
	GLFWwindow *window = glfwCreateWindow(width, height, "hamster", NULL, NULL);

	while(!glfwWindowShouldClose(window))
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}