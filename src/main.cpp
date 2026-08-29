#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.hpp"

constexpr int checkerSize = 8;

const char *vertexShaderSource = "#version 330 core\n"
  "layout (location = 0) in vec3 aPos;\n"
  "layout (location = 1) in vec2 aTexCoord;\n"
  "out vec2 TexCoord;\n"
  "void main()\n"
  "{\n"
  "  gl_Position = vec4(aPos, 1.0);\n"
  "  TexCoord = aTexCoord;\n"
  "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
  "out vec4 FragColor;\n"
  "in vec2 TexCoord;\n"
  "uniform sampler2D ourTexture;\n"
  "void main()\n"
  "{\n"
  "  FragColor = texture(ourTexture, TexCoord);\n"
  "}\0";

void closeOnEscape(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

int run(GLFWwindow* window) {
  std::cout << glGetString(GL_VERSION) << '\n';

  float vertices[] = {
      0.5f,    0.5f,   0.0f,   1.0f,  1.0f,// top-right
      0.5f,   -0.5f,   0.0f,   1.0f,  0.0f,// bottom-right
    -0.5f,  -0.5f,  0.0f,  0.0f, 0.0f,// bottom-left
    -0.5f,   0.5f,  0.0f,  0.0f, 1.0f// top-left
  };

  unsigned int indices[] = {
    0, 1, 3, // first-triangle 
    1, 2, 3 // second-triangle
  };

  unsigned char checkerBoard[checkerSize][checkerSize][3];

  for(int row=0; row<checkerSize; ++row) {
    for(int col=0; col<checkerSize; ++col) {
      unsigned char value = ((row+col) % 2 == 0) ? 255 : 0;
      for(int channel=0; channel<3; ++channel) checkerBoard[row][col][channel] = value;
    }
  }

  Shader shader(vertexShaderSource, fragmentShaderSource);
  if(!shader.valid()) { return -1; }

  unsigned int VBO, VAO, EBO;
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, checkerSize, checkerSize, 0, GL_RGB, GL_UNSIGNED_BYTE, checkerBoard);

  while(!glfwWindowShouldClose(window)) {
    closeOnEscape(window);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  return 0;
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

  int result = run(window);
  glfwTerminate();
  return result;
}