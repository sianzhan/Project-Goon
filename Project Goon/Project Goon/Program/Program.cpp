#include "Program.h"
#include <fstream>
#include <iostream>

static const char *exec = "Program: ";
void Program::init()
{
	program = glCreateProgram();
}

void Program::loadShader(GLenum shader_type, std::string filepath)
{
	if (!program)
	{
		throw std::runtime_error("Please create program before loading!");
	}
	std::ios::sync_with_stdio(false);
	std::ifstream ifs(filepath, std::ifstream::in);
	if (!ifs)
	{
		throw std::runtime_error(std::string("Unable to load shader file ") + filepath + "!");
	}

	std::string file_data;
	ifs.seekg(0, std::ios::end);
	file_data.resize(ifs.tellg());
	ifs.seekg(0, std::ios::beg);
	ifs.read(&file_data.front(), file_data.size());
	ifs.close();

	GLuint shader = glCreateShader(shader_type), *shader_to_be_loaded = 0;
	const GLchar* shader_source = file_data.c_str();
	glShaderSource(shader, 1, &shader_source, NULL);

	glCompileShader(shader);

	char msg[1001];
	GLint verdict;
	glGetShaderInfoLog(shader, 1000, 0, msg); //Print Compile Message
	std::cout << msg;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &verdict);
	if(verdict == GL_FALSE)
		throw std::runtime_error("Shader Compilation Error!");

	switch (shader_type)
	{
	case GL_VERTEX_SHADER: shader_to_be_loaded = &vertex_shader; break;
	case GL_FRAGMENT_SHADER: shader_to_be_loaded = &fragment_shader; break;
	case GL_GEOMETRY_SHADER: shader_to_be_loaded = &geometry_shader; break;
	default: std::runtime_error("Shader type not supported!");
	}

	if (*shader_to_be_loaded != 0)
	{
		glDetachShader(program, *shader_to_be_loaded);
		glDeleteShader(*shader_to_be_loaded);
	}
	*shader_to_be_loaded = shader;
	if (shader != 0)
	{
		glAttachShader(program, *shader_to_be_loaded);
		std::cout << exec << "GLSL File(\'" << filepath << "\') loaded!" << std::endl;
	}
	else
	{
		std::cout << exec << "GLSL File(\'" << filepath << "\') invalid shader!" << std::endl;
	}
}

void Program::link()
{
	glLinkProgram(program);
}

void Program::use()
{
	glUseProgram(program);
}