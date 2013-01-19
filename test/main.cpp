#include <lerna.h>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <cstdio>

bool run = true;

void GLFWCALL key(int k, int action) {
  if(action!=GLFW_PRESS) return;
  if(k==GLFW_KEY_ESC){
    run = false;
  }
}

int main(int argc, char** argv) {
  if(!glfwInit()) return -1;

  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  if(LERNA_OK == lernaInit()){
    if(glfwOpenWindow(200, 200, 0,0,0,0,16,0, GLFW_WINDOW)) {
      glewInit();
      glfwSetKeyCallback(key);

      glClearColor(0.7f, 0.7f, 0.7f, 1.f);

      while(run){
        glClear(GL_COLOR_BUFFER_BIT);


        glfwSwapBuffers();
      }
    }
    lernaExit();
  }

  glfwTerminate();
  return 0;
}
