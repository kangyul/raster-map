#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <format>

#include "mercator.hpp"
#include "shader.hpp"
#include "stb_image.h"

struct NDCRect {
  float offsetX, offsetY, scaleX, scaleY;
};

struct Tile {
  TileId id;
  unsigned int texture;
};

const char *vertexShaderSource = "#version 330 core\n"
  "layout (location = 0) in vec3 aPos;\n"
  "layout (location = 1) in vec2 aTexCoord;\n"
  "uniform vec4 uTileRect; // xy = offset, zw = scale\n"
  "out vec2 TexCoord;\n"
  "void main()\n"
  "{\n"
  "  gl_Position = vec4(aPos.xy * uTileRect.zw + uTileRect.xy, 0.0, 1.0);\n"
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

NDCRect tileToNDC(TileId tile) {
  int n = 1 << tile.z;
  float scaleX =  2.0 / n;
  float scaleY = -2.0 / n; // The one y flip in the pipeline: tile y grows south, NDC y grows north.
  float offsetX = 2.0 * tile.x / n - 1.0;
  float offsetY = 1.0 - 2.0 * tile.y / n;
  return {.offsetX = offsetX, .offsetY = offsetY, .scaleX = scaleX, .scaleY = scaleY};
}

unsigned int loadTexture(const char* path) {
  int width, height;
  unsigned char *data = stbi_load(path, &width, &height, nullptr, 4);
  if (!data) {
    std::cerr << "Failed to load texture " << path << ": " << stbi_failure_reason() << std::endl;
    return 0;
  }

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);

  return texture;
}

int run(GLFWwindow* window) {
  std::cout << glGetString(GL_VERSION) << '\n';

  // Quad in tile-local space: [0, 1] on both axes, y down - the direction PNG rows
  // and the world coordinate both run. Hence texture coords equal position: same space.
  // NDC has y up, so the flip belongs to the tile-to-NDC transform, not here.
  float vertices[] = {
      1.0f,    0.0f,   0.0f,   1.0f,  0.0f,// top-right
      1.0f,    1.0f,   0.0f,   1.0f,  1.0f,// bottom-right
     0.0f,   1.0f,  0.0f,  0.0f, 1.0f,// bottom-left
     0.0f,   0.0f,  0.0f,  0.0f, 0.0f// top-left
  };

  unsigned int indices[] = {
    0, 1, 3, // first-triangle 
    1, 2, 3 // second-triangle
  };

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

  shader.use();
  const int loc = glGetUniformLocation(shader.getShaderProgram(), "uTileRect");
  if (loc == -1) { 
    std::cerr << "uniform:uTileRect not found!" << std::endl;
    return -1;
  }

  std::vector<Tile> tiles;
  tiles.reserve(4);
  for(int x=0; x<2; ++x) {
    for(int y=0; y<2; ++y) {
      std::string textureLoc = std::format("tiles/1/{}/{}.png", x, y);
      unsigned int textureId = loadTexture(textureLoc.c_str());
      if (textureId == 0) { return -1; }
      TileId tileId{.z=1, .x=x, .y=y};
      tiles.push_back({.id = tileId, .texture = textureId});
    }
  }

  while(!glfwWindowShouldClose(window)) {
    closeOnEscape(window);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    shader.use();
    glBindVertexArray(VAO);

    for(const Tile& tile : tiles) {
      NDCRect ndc = tileToNDC(tile.id);
      glUniform4f(loc, ndc.offsetX, ndc.offsetY, ndc.scaleX, ndc.scaleY);
      glBindTexture(GL_TEXTURE_2D, tile.texture);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    }
    
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