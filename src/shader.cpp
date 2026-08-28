#include "shader.hpp"

#include <OpenGL/gl3.h>
#include <iostream>

Shader::Shader(const char* vertexShaderSource, const char* fragmentShaderSource) {
  unsigned int vertexShader, fragmentShader;

  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);
  bool vsOk = shaderCompiled(vertexShader, "vertex");

  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);
  bool fsOk = shaderCompiled(fragmentShader, "fragment");

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  ok_ = vsOk && fsOk && programLinked(shaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

Shader::~Shader() { glDeleteProgram(shaderProgram); }

void Shader::use() const { glUseProgram(shaderProgram); }

bool Shader::valid() const { return ok_; }

bool Shader::shaderCompiled(unsigned int shader, const char* name) {
  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << name << " shader failed to compile:\n" << log;
  }
  return success;
}

bool Shader::programLinked(unsigned int program) {
  int success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    std::cerr << "shader program failed to link:\n" << log;
  }
  return success;
}