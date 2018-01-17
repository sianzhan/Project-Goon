#pragma once

#include <GL/glew.h>
#include <time.h>
#include "../Texture/Texture.h"

class Program 
{
private:
	std::string vertex_shader_src;
	std::string fragment_shader_src;
	std::string geometry_shader_src;
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
	void reload();
	GLuint data() { return program; };
};