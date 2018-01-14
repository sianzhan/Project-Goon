#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <vector>
#include <map>
#include <string>
#include "ObjLoader.h"
#include <glm/gtx/quaternion.hpp>

//Robot must only be created after GLEW has already initialized
class Robot
{
private:
	static const mat4 identity4;
	static std::string exec;
	struct Part
	{
		std::string name;
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

		vec3 pivotCoord = vec3(0, 0, 0);
		const Part * pivotPart = nullptr;
		mat4 rawModel = mat4(1.0); //Model Matrix for every part, without applying effects
		
		//Model Matrix for every part, with applying effects
		//0: current Model, 1: nextModel
		vec3 t_Tra[2] = { vec3(0), vec3(0) };
		vec3 t_Sca[2] = { vec3(1.0), vec3(1.0) };
		quat t_Rot[2] = { quat(1, 0, 0, 0) };
		mat4 Model = mat4(1.0);
	
		//Convenient Constructor
		Part(std::string name, std::string obj) : name(name), obj_source(obj) {}
		Part(std::string name, std::string obj, std::string mtl) : name(name), obj_source(obj), mtl_source(mtl) {}
		
		int frame[2] = { -1, -1 }; //0: current frame, 1: next frame
		float time_offset = 0;

		const mat4 getModel() const;
	};

	GLuint uni_mtl_id = 0;
	GLuint unb_mvp_id = 0;
	std::vector<Part> parts;
	std::map<std::string, Part*> ref_parts;

	GLuint vao = 0;
	GLuint vbo_vertices = 0;
	GLuint vbo_uvs = 0;
	GLuint vbo_normals = 0;
	GLuint ubo_mvp = 0;

	struct Script
	{
		struct Keyframe
		{
			struct Effect {
				enum Type{	
					SCALE,
					TRANSLATE,
					ROTATE
				};
				Type type;
				vec3 xyz;
				float theta;
			};
			float time;
			std::vector<Effect> effects;
		};
		std::string name;
		std::map<Part*, std::vector<Keyframe>> keyframes;
		float max_time = 0;
		bool loop = 1;
	};

	std::string struct_src;
	std::string script_src;
	std::map<std::string, Script> actions;

	Script *action = nullptr;

	bool loadStructure(std::string struct_path);
	void loadPart2Buffer();
	void loadRobotInfo(std::string robot_path);
	bool loadScript(std::string script_path);
public:
	void init(std::string robot_path);
	void reload();
	void render();
	void update(float);
};