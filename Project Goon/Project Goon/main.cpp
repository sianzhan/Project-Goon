#define GLEW_STATIC
#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <GLUT/glut.h>
#include <fstream>
#include <time.h>
#include "Texture/Texture.h"
#include "Program/Program.h"
#include "Robot.h"
#include <glm/gtc/matrix_transform.hpp>
#include "main.h"

Program program;
mat4 Projection;
mat4 View;
mat4 Model;

Robot robot;

void initGL();
void initShader();
void draw();
void initShader();
void update(int);

int texId = -1;

int main(int argc, char* argv[])
{
	printf("TEST\n");
	try
	{
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
		glutInitWindowPosition(100, 100);
		glutInitWindowSize(600, 600);
		glutCreateWindow("Wkwkwkwk");

		GLenum err = glewInit();
		if (GLEW_OK != err) {
			fprintf(stderr, "GLEW Error:%s\n", glewGetErrorString(err));
			return 1;
		}

		glutDisplayFunc(draw);
		glutTimerFunc(0, update, 0);
		initGL();
		initShader();
		std::cout << "GLUT: Drawing" << std::endl;
		glutMainLoop();
	}
	catch (std::exception e)
	{
		printf("%s\n", e.what());
		terminate();
	}
}

void initGL()
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	texId = Texture::GenTexture("troll.png");
	Projection = perspective(80.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	View = lookAt(
		glm::vec3(0, 0, 5), // Camera is at (0,10,25), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, -1, 0)  // Head is up (set to 0,1,0 to look upside-down)
	);
}

void initShader()
{
	program.init();
	program.loadShader(GL_VERTEX_SHADER, "s1.vert");
	program.loadShader(GL_FRAGMENT_SHADER, "s1.frag");
	program.link();
	program.use();


	robot.init();
}


#define PI 3.14159
void draw()
{
	glCullFace(GL_FRONT);

	GLfloat red[] = { 1.0f, 0.8f, 0.8f, 1.0f };
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//glClearBufferfv(GL_COLOR, 0, red);

	robot.render();

	glutSwapBuffers();

}

void update(int val) {
	draw();
	glutPostRedisplay();
	glutTimerFunc(10, update, 0);
}