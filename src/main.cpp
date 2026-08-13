#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include <iostream>

void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

int main() {
  if (!glfwInit()) { return -1; }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  
  GLFWwindow* window = glfwCreateWindow(800, 600, "raster-map", nullptr, nullptr);
  if (!window) { 
    glfwTerminate();  
    return -1;
  }

  glfwMakeContextCurrent(window);

  std::cout << glGetString(GL_VERSION) << '\n';

  while(!glfwWindowShouldClose(window)) {
    processInput(window);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}