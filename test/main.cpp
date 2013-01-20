#include <lerna.h>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <cstdio>

bool run = true;
unsigned int winheight = 600, winwidth = 800;

void init3D() {
  glClearColor(0.7f, 0.7f, 0.7f, 1.f);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(90.f, 800.f/600.f, 0.1f, 15.f);
  glMatrixMode(GL_MODELVIEW);

  glPointSize(4);
  glLineWidth(4);
}

lernaDualControllerData ldcd;
float mat[16]={1.f, 0.f, 0.f, 0.f,
              0.f, 1.f, 0.f, 0.f,
              0.f, 0.f, 1.f, 0.f,
              0.f, 0.f, 0.f, 1.f};
void draw3D() {
  glLoadIdentity();
  glTranslatef(0.f, -1.f, -3.f);

  if(LERNA_OK == lernaGetDualControllerData(&ldcd)) {
    for(int i=0; i<2; i++){
      //TODO Extract rotation
      //Position
      mat[12] = ldcd.data[i].pos[0];
      mat[13] = ldcd.data[i].pos[1];
      mat[14] = ldcd.data[i].pos[2];
      glPushMatrix();
      glMultMatrixf((const float*)&mat);
      glBegin(GL_LINES);
      glVertex3f(0.f, 0.f, 0.f);
      glVertex3f(0.f, 0.f, -1.f);
      glEnd();
      glPopMatrix();
    }
  }

  glBegin(GL_POINTS);
  glColor3f(0.f, 0.f, 0.f);
  glVertex3f(0.f, 0.f, 0.f);
  glEnd();
}

void GLFWCALL key(int k, int action) {
  if(action!=GLFW_PRESS) return;
  if(k==GLFW_KEY_ESC){
    run = false;
  }
}

void GLFWCALL resize(int width, int height) {
  glMatrixMode(GL_PROJECTION);
  if(height>0) {
    glLoadIdentity();
    gluPerspective(90.f, (float)width/(float)height, 0.1f, 15.f);
  }
  glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
  if(!glfwInit()) return -1;

  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 2);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 1);

  if(LERNA_OK == lernaInit()){
    if(glfwOpenWindow(800, 600, 0,0,0,0,16,0, GLFW_WINDOW)) {
      glewInit();
      glfwSetKeyCallback(key);
      glfwSetWindowSizeCallback(resize);
      glGetError();

      init3D();

      while(run){
        glClear(GL_COLOR_BUFFER_BIT);
        draw3D();

        glfwSwapBuffers();

        unsigned int err = glGetError();
        if(err) printf("%i\n", err);
      }
    }
    lernaExit();
  }

  glfwTerminate();
  return 0;
}
