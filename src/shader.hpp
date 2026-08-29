#pragma once

class Shader {
public:
  Shader(const char* vertexShaderSource, const char* fragmentShaderSource);
  ~Shader();
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  void use() const;
  bool valid() const;

  unsigned int& getShaderProgram() { return shaderProgram; }

private:
  unsigned int shaderProgram = 0;
  bool ok_ = false;
  static bool shaderCompiled(unsigned int, const char*);
  static bool programLinked(unsigned int);
};