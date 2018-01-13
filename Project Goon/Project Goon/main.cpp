#define GLEW_STATIC
#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <GLUT/glut.h>
#include <fstream>
#include <time.h>
#include "Robot.h"
#include "Texture/Texture.h"
#include "Program/Program.h"
#include <glm/gtc/matrix_transform.hpp>
#include "main.h"

Program program;
mat4 Projection;
mat4 View;

Robot robot;

void init();
void draw();
void reshape(int, int);
void keyboard(unsigned char, int, int);
void update(int);

int main(int argc, char* argv[])
{
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
		glutReshapeFunc(reshape);
		glutKeyboardFunc(keyboard);
		glutTimerFunc(0, update, 0);
		init();

		std::cout << "GLUT: Drawing\n" << std::endl;
		glutMainLoop();

	}
	catch (std::exception e)
	{
		printf("%s\n", e.what());
		terminate();
	}
}

void init()
{
	//initGL
	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	//init Shader
	program.init();
	program.loadShader(GL_VERTEX_SHADER, "s1.vert");
	program.loadShader(GL_FRAGMENT_SHADER, "s1.frag");
	program.loadShader(GL_GEOMETRY_SHADER, "s1.geo");
	program.link();
	program.use();

	std::cout << std::endl;
	robot.init("robot.txt");
}

void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	Projection = perspective(radians(60.0f), (float)width/height, 0.1f, 300.0f);
}

void draw()
{
	glCullFace(GL_FRONT);

	GLfloat red[] = { 1.0f, 0.8f, 0.8f, 1.0f };
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//glClearBufferfv(GL_COLOR, 0, red);

	robot.render();

	glutSwapBuffers();

}

void keyboard(unsigned char key, int, int)
{
	if (key == 'r') robot.reload();
}

void update(int val) {
	draw();
	glutPostRedisplay();
	glutTimerFunc(20, update, 0);
}