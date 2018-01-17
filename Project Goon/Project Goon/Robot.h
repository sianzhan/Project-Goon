#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <vector>
#include <map>
#include <string>
#include "ObjLoader.h"
#include "Program/Program.h"
#include <glm/gtx/quaternion.hpp>

//Robot must only be created after GLEW has already initialized
class Robot
{
private:
	static const mat4 identity4;
	static std::string exec;
	
	std::vector<Program> programs;
	int active_program_idx = 0;
	struct Part;
	struct Script
	{
		enum Effect {
			SCALE,
			TRANSLATE,
			ROTATE
		};
		struct Keyframe {
			float time;
			union {
				vec3 xyz;
				quat rot;
			};
		};
		std::string name;
		std::map<Part*, std::map<Effect, std::vector<Keyframe>>> effects;
		float max_time = 0;
		bool loop = 1;
	};
	const static Script::Effect effect_types[];
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
		vec3 t_Tra[2];
		vec3 t_Sca[2];
		quat t_Rot[2];
		mat4 Model = mat4(1.0);
	
		//Convenient Constructor
		Part(std::string name, std::string obj) : name(name), obj_source(obj) {}
		Part(std::string name, std::string obj, std::string mtl) : name(name), obj_source(obj), mtl_source(mtl) {}
		
		std::map<Script::Effect, int[2]> effects_indices;
		std::map<Script::Effect, float> time_offsets;

		const mat4 getModel() const;
	};
	Part* head = nullptr;
	vec3 eye_pos;
	vec3 look_pos;
	vec3 god_pos;

	std::vector<Part> parts;
	std::map<std::string, Part*> ref_parts;

	GLuint uni_mtl_id = 0;
	GLuint uni_time_id = 0;
	GLuint uni_mvp_depth_id = 0;
	GLuint unb_mvp_id = 0;

	GLuint vao = 0;
	GLuint vbo_vertices = 0;
	GLuint vbo_uvs = 0;
	GLuint vbo_normals = 0;
	GLuint ubo_mvp = 0;
	GLint ubo_mvp_size = 0;

	std::string struct_src;
	std::string script_src;
	std::string shader_src;

	std::map<std::string, Script> actions;

	Script *action = nullptr;
	double frame_count = 0;

	void preload();
	bool loadStructure(std::string struct_path);
	void loadPart2Buffer();
	void loadRobotInfo(std::string robot_path);
	void loadShaders(std::string shader_path);
	bool loadScript(std::string script_path);
public:
	~Robot();
	void init(const char* robot_path);
	void destroy();
	void reload();
	void render(bool = 1);
	void update();
	void act(std::string);
	void updateProgram();
	void nextProgram();
	void prevProgram();
	std::tuple<vec3, vec3, vec3> getHeadPos();
};