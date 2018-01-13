#pragma once

#include <GL/glew.h>
#include <GLUT/glut.h>
#include <time.h>
#include "../Texture/Texture.h"

class Program 
{
private:
	GLuint program = 0;
	GLuint vertex_shader = 0;
	GLuint geometry_shader = 0;
	GLuint fragment_shader = 0;
public:
	//Must init after glew has loaded
	void init();
	void loadShader(GLenum shader_type, std::string filepath);
	void link();
	void use();
	GLuint data() { return program; };
};