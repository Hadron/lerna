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
  glClearDepth(1.f);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
}

lernaDualControllerData ldcd;
float mat[16]={1.f, 0.f, 0.f, 0.f,
              0.f, 1.f, 0.f, 0.f,
              0.f, 0.f, 1.f, 0.f,
              0.f, 0.f, 0.f, 1.f};
void draw3D() {
  glLoadIdentity();
  glTranslatef(0.f, -1.f, -3.f);
  //glPushMatrix();
  //glRotatef(-90.f, 1.f, 0.f, 0.f);

  if(LERNA_OK == lernaGetDualControllerData(&ldcd)) {
    for(int i=0; i<2; i++){
      //Rotation
#define Q ldcd.data[i].quat
      mat[0] = 1.f - 2.f*Q[2]*Q[2] - 2.f*Q[3]*Q[3];
      mat[1] = 2.f*Q[1]*Q[2] + 2.f*Q[0]*Q[3];
      mat[2] = 2.f*Q[1]*Q[3] - 2.f*Q[0]*Q[2];

      mat[4] = 2.f*Q[1]*Q[2] - 2.f*Q[0]*Q[3];
      mat[5] = 1.f - 2.f*Q[1]*Q[1] - 2.f*Q[3]*Q[3];
      mat[6] = 2.f*Q[2]*Q[3] + 2.f*Q[0]*Q[1];

      mat[8] = 2.f*Q[1]*Q[3] + 2.f*Q[0]*Q[2];
      mat[9] = 2.f*Q[2]*Q[3] - 2.f*Q[0]*Q[1];
      mat[10] = 1.f - 2.f*Q[1]*Q[1] - 2.f*Q[2]*Q[2];
#undef Q
      //Position
      mat[12] = ldcd.data[i].pos[0];
      mat[13] = ldcd.data[i].pos[1];
      mat[14] = ldcd.data[i].pos[2];
      glPushMatrix();
      glMultMatrixf((const float*)&mat);
      glBegin(GL_TRIANGLES);
      glColor3f(0.f, 0.f, 1.f);
      glVertex3f(0.1f, 0.f, 0.5f);
      glVertex3f(-0.1f, 0.f, 0.5f);
      glVertex3f(0.f, -0.2f, 0.5f);

      glColor3f(1.f, 0.f, 0.f);
      glVertex3f(0.2f, 0.f, 0.f);
      glVertex3f(-0.2f, 0.f, 0.f);
      glColor3f(0.f, 1.f, 0.f);
      glVertex3f(0.f, -0.5f, 0.f);
      glEnd();
      glPopMatrix();
    }
  }
  //glPopMatrix();

  glBegin(GL_LINES);
  glColor3f(1.f, 0.f, 0.f);
  glVertex3f(0.f, 0.f, 0.f);
  glVertex3f(0.5f, 0.f, 0.f);
  glColor3f(0.f, 1.f, 0.f);
  glVertex3f(0.f, 0.f, 0.f);
  glVertex3f(0.f, 0.5f, 0.f);
  glColor3f(0.f, 0.f, 1.f);
  glVertex3f(0.f, 0.f, 0.f);
  glVertex3f(0.f, 0.f, 0.5f);
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
