#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <map>
#include <format>
#include <string>
#include <algorithm>

#include "mercator.hpp"
#include "shader.hpp"
#include "stb_image.h"
#include "camera.hpp"

struct InputState {
  double scrollY = 0.0;
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

  std::map<TileId, unsigned int> tiles;

  Camera camera{{0.5, 0.5}, 1.0};

  bool dragging = false;
  double lastX, lastY;

  glfwGetCursorPos(window, &lastX, &lastY);

  InputState input;
  glfwSetWindowUserPointer(window, &input);

  glfwSetScrollCallback(window, [](GLFWwindow* w, double /*xoff*/, double yoff) {
    auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(w));
    input->scrollY += yoff;
  });

  while(!glfwWindowShouldClose(window)) {
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    int mouseButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    double lastS = pixelsPerWorldUnit(camera.zoom);
    camera.zoom = std::max(camera.zoom + input.scrollY * 0.05, 0.0);
    input.scrollY = 0.0;
    double currS = pixelsPerWorldUnit(camera.zoom);

    double cursorX, cursorY;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    double deltaX = cursorX - (windowWidth  / 2.0);
    double deltaY = cursorY - (windowHeight / 2.0);

    double mx = deltaX * fbWidth  / windowWidth;
    double my = deltaY * fbHeight / windowHeight;

    camera.center.x += mx/lastS - mx/currS;
    camera.center.y += my/lastS - my/currS;

    if(!dragging) {
      if(mouseButtonState == GLFW_PRESS) {
        dragging = true;
        glfwGetCursorPos(window, &lastX, &lastY);
      }
    } else {
      if(mouseButtonState == GLFW_PRESS) {
        double currX, currY;
        glfwGetCursorPos(window, &currX, &currY);
        double deltaX = currX - lastX;
        double deltaY = currY - lastY;

        double worldDX = deltaX * fbWidth  / windowWidth  / pixelsPerWorldUnit(camera.zoom);
        double worldDY = deltaY * fbHeight / windowHeight / pixelsPerWorldUnit(camera.zoom);
        camera.center.x -= worldDX;
        camera.center.y -= worldDY;

        lastX = currX;
        lastY = currY;
      } else if(mouseButtonState == GLFW_RELEASE) { dragging = false; }
    }

    closeOnEscape(window);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    glBindVertexArray(VAO);

    for(const TileId& tile : visibleTiles(camera, fbWidth, fbHeight)) {
      if (tiles.find(tile) == tiles.end()) {
        std::string textureLoc = std::format("tiles/{}/{}/{}.png", tile.z, tile.x, tile.y);
        unsigned int textureId = loadTexture(textureLoc.c_str());
        tiles[tile] = textureId;
      }

      if (tiles[tile] == 0) { continue; }

      NDCRect ndc = tileToNDC(tile, camera, fbWidth, fbHeight);
      glUniform4f(loc, ndc.offsetX, ndc.offsetY, ndc.scaleX, ndc.scaleY);
      glBindTexture(GL_TEXTURE_2D, tiles[tile]);
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
