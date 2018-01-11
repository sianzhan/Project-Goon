#pragma once
#include <GL/glew.h>
#include <vector>
#include <map>
#include <string>
#include "ObjLoader.h"

//Robot must only be created after GLEW has already initialized
class Robot
{
private:
	static std::string exec;
	struct Part
	{
		std::string obj_source;
		std::string mtl_source;

		//Offset in VBO (vertices, uvs, normals)
		//3 vertices -> 1 face (Triangulated)
		GLuint count_vertices;

		//this holds the material infomations of faces
		//--> render 'n' faces with specific material.
		std::vector<MtlInfo> mtl_infos; 
		
		//this hold the material data required to render
		std::map<std::string, Material> mtl;

		//Convenient Constructor
		Part(const char *obj) : obj_source(obj) {}
		Part(const char *obj, const char *mtl) : obj_source(obj), mtl_source(mtl) {}
	};

	GLuint uni_mtl_id = 0;
	GLuint unb_vp_id = 0;
	std::vector<Part> parts;

	GLuint vao = 0;
	GLuint vbo_vertices = 0;
	GLuint vbo_uvs = 0;
	GLuint vbo_normals = 0;
	GLuint ubo_vp = 0;


public:
	void init();
	void render();
};