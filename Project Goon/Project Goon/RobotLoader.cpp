#include "Robot.h"
#include "Texture/Texture.h"
#include "main.h"
#include <fstream>
#include <sstream>
#include <stack>
#include <glm/gtc/matrix_transform.hpp>

//Load Robot Informations (Part name, Part src, Material src)
//
//Format:
//<part_name> <part_obj_src> <part_mtl_src>
//'.' for break, '#' for comment
void Robot::loadRobotInfo(std::string robot_path)
{
	std::ifstream ifs(robot_path, std::ios_base::in);
	if (!ifs)
	{
		std::cout << exec << "Robot file(\'" + robot_path + "\' not found!" << std::endl;
		return;
	}
	else
	{
		std::cout << exec << "Loading Robot Info from \'" << robot_path << "\'" << std::endl;
	}
	std::string line, cmd, src1, shader_type, src2;
	while (std::getline(ifs, line))
	{
		if (line.empty() || line.front() == '#') continue;
		std::stringstream ss(line);
		if (ss.peek() == '.') break;
		if (ss >> cmd)
		{
			if(cmd == "struct" && ss >> src1)
				struct_src = src1;
			else if(cmd == "script" && ss >> src1)
				script_src = src1;
			else if (cmd == "shader" && ss >> src1)
				shader_src = src1;
			else if(ss >> src1 >> src2)
				//first src is obj_src, second is mtl_src, and cmd is the part name
				parts.push_back(Part(cmd, src1, src2)); 
		}
				
	}
	std::cout << exec << "Loaded Robot Info from \'" << robot_path << "\'" << std::endl;
	std::cout << exec << "Crafting the robot...\n" << std::endl;

	//Put it down here instead of assigning it above because,
	//when vector performs resizing, the address changes, and thus BOOM.
	for (int i = 0; i < parts.size(); ++i) ref_parts[parts[i].name] = &parts[i];
}

void Robot::loadShaders(std::string shader_path)
{
	if (shader_path.empty())
	{
		std::cout << exec << "Please specific shader src by \'shader <shaderlist_src>\'!" << std::endl;
		return;
	}
	std::ifstream ifs(shader_path, std::ios_base::in);
	if (!ifs)
	{
		std::cout << exec << "Shader List file(\'" + shader_path + "\' not found!" << std::endl;
		return;
	}
	else
	{
		std::cout << exec << "Loading Shader List from \'" << shader_path << "\'" << std::endl;
	}
	std::string line, src, shader_type;
	while (std::getline(ifs, line))
	{
		if (line.empty() || line.front() == '#') continue;
		else if (line.front() == '.') break;
		std::stringstream ss(line);

		programs.push_back(Program());
		Program &program = programs.back();
		program.init();
		while (ss >> shader_type >> src)
		{
			if (shader_type == "GL_VERTEX_SHADER")
				program.loadShader(GL_VERTEX_SHADER, src);
			else if (shader_type == "GL_FRAGMENT_SHADER")
				program.loadShader(GL_FRAGMENT_SHADER, src);
			else if (shader_type == "GL_GEOMETRY_SHADER")
				program.loadShader(GL_GEOMETRY_SHADER, src);
		}
		program.link();
	}
	std::cout << exec << "Loaded Shaders from \'" << shader_path << "\'" << std::endl;
}

//Pre: Robot informations(Parts) must have already been loaded by loadRobotInfo()
//Load Vertices/UVs/Normals/MaterialInformations/Materials from for every parts into Graphic Buffer(VBO,UBO)
//Obj, Mtl files must exists under as-specific directory
//Only execute this after GLEW has been loaded.
void Robot::loadPart2Buffer()
{
	//Load Parts from Obj
	std::vector<vec3> out_vertices;
	std::vector<vec2> out_uvs;
	std::vector<vec3> out_normals;

	//Loop through every part informations, load their obj and mtl
	int last_face_count = 0;
	for (int i = 0; i < parts.size(); ++i)
	{
		Part &part = parts[i];
		std::vector<MtlInfo> out_mtl_infos; //moved in so that it won't stack together

		//Load Material Data
		if (!part.mtl_source.empty()) loadMtl(part.mtl_source, part.mtl);

		//Load textures for material
		for (std::map<std::string, Material>::iterator it = part.mtl.begin();
			it != part.mtl.end(); it++)
		{
			Material &mtl = it->second;
			if (mtl.tex_Kd == 0 && !mtl.map_Kd.empty())
			{
				mtl.tex_Kd = Texture::GenTexture(mtl.map_Kd.c_str());
			}
		}

		//Load Faces from OBJ
		loadObj(part.obj_source, out_vertices, out_uvs, out_normals, out_mtl_infos);
		part.count_vertices = out_vertices.size();
		part.mtl_infos = out_mtl_infos;
		if (i > 0)
		{
			part.count_vertices -= last_face_count;
		}
		std::cout << std::endl;
		last_face_count = out_vertices.size();
	}
	
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * out_vertices.size(), out_vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * out_uvs.size(), out_uvs.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * out_normals.size(), out_normals.data(), GL_STATIC_DRAW);


	//Unbind Buffers
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}

//Pre: Robot Informations(Parts) must already been loaded by loadRobotInfo()
//Craft the robot by structuring & modelling the parts' pivot, location, etc
//by manipulating their Model Matrix
//
//Format:
//usePart <part_name> : select part set
//pivot <x> <y> <z> : set pivot
//translate <x> <y> <z> : translate part
//scale <x> <y> <z> : scale part
//rotate <angle> <x> <y> <z> : rotate part by angle in degree form about origin(0, 0, 0) 
//pushMatrix : push the current select part as pivot
//popMatrix : pop the pivot matrix
//'.' for break, '#' for comment
bool Robot::loadStructure(std::string struct_path)
{
	this->struct_src = struct_path;
	std::ifstream ifs(struct_path, std::ios_base::in);
	if (!ifs)
	{
		std::cout << exec << "Structure file(\'" + struct_path + "\' not found!" << std::endl;
		return false;
	}
	else
	{
		std::cout << exec << "Loading robot structure(\'" << struct_path << "\')" << std::endl;
	}
	for (int i = 0; i < parts.size(); ++i) {
		parts[i].rawModel = parts[i].Model = identity4;
		parts[i].pivotCoord = vec3(0.0f);
		parts[i].pivotPart = nullptr;
	}
	std::string cmd, line;
	std::stack<const Part * > stack_part;
	Part *ptr_part = nullptr;
	mat4 *ptr_Model = nullptr, *ptr_raw_Model = nullptr;
	vec3 pivot;
	float x = 0, y = 0, z = 0, theta = 0;
	std::string part_name;
	bool err = 0;
	int line_counter = 0;
	while (std::getline(ifs, line))
	{
		line_counter++;
		if (line.empty() || line.front() == '#') continue;
		if (line.front() == '.') break;
		std::stringstream ss(line);

		ss >> cmd;
		if (cmd == "usePart" && ss >> part_name)
		{
			//place back pivot translation to previous model
			if (ptr_Model)
			{
				*ptr_Model = translate(*ptr_Model, -pivot);
				*ptr_raw_Model = *ptr_Model; //update the value of rawModel to Model
			}
			auto it = ref_parts.find(part_name);
			if (it == ref_parts.end())
			{
				std::cout << '(' << struct_path << ':' << line_counter
					<< ") Invalid part name: \'" << part_name << "\'\n";
				ptr_part = nullptr;
				ptr_Model = nullptr;
				continue;
			}
			ptr_part = it->second;
			Part &part = *it->second;
			ptr_Model = &part.Model;
			ptr_raw_Model = &part.rawModel;
			pivot = vec3(0);
			if (!stack_part.empty() && stack_part.top() != ptr_part)
				part.pivotPart = stack_part.top();
			else part.pivotPart = nullptr;
		}
		else if (ptr_part == nullptr)
		{
			std::cout << '(' << struct_path << ':' << line_counter
				<< ") Please load a part using \"usePart <part_id>\" first!\n";
		}
		else if (cmd == "head" && ss >> x >> y >> z) {
			head = ptr_part;
			eye_pos = vec3(x, y, z);
			if (ss >> x >> y >> z) look_pos = vec3(x, y, z);
			if (ss >> x >> y >> z) god_pos = vec3(x, y, z);
		}
		else if (cmd == "pushMatrix") //setup Pivot Part
		{
			stack_part.push(ptr_part);
		}
		else if (cmd == "popMatrix")
		{
			if(!stack_part.empty()) stack_part.pop();
		}
		else if (cmd == "pivot" && ss >> x >> y >> z)
		{
			*ptr_Model = translate(*ptr_Model, -pivot);
			pivot = vec3(x, y, z);
			ptr_part->pivotCoord = pivot;
			*ptr_Model = translate(*ptr_Model, pivot);
		}
		else if (cmd == "rotate" && ss >> theta >> x >> y >> z)
		{
			*ptr_Model = rotate(*ptr_Model, radians(theta), vec3(x, y, z));
		}
		else if (cmd == "translate" && ss >> x >> y >> z)
		{
			*ptr_Model = translate(*ptr_Model, vec3(x, y, z));
		}
		else if (cmd == "scale" && ss >> x >> y >> z)
		{
			*ptr_Model = scale(*ptr_Model, vec3(x, y, z));
		}
		else
		{
			std::cout << '(' << struct_path << ':' << line_counter
				<< ") Command: \'" + line + "\' invalid!\n";
		}
	}
	if (ptr_Model) {
		*ptr_Model = translate(*ptr_Model, -pivot);
		*ptr_raw_Model = *ptr_Model; //update the value of rawModel to Model
	}
	std::cout << exec << "Loaded robot structure(\'" << struct_path << "\')\n" << std::endl;
	return true;
}

//Pre: Robot Informations(Parts) must already been loaded by loadRobotInfo()
//Action per Part per Effect per Keyframe based ANIMATOR with SMOOTH TRANSITIONING!
//
//Format:
//time set current time, must be in incremental order
//usePart <part_name> : select part set
//translate <x> <y> <z> : translate part
//scale <x> <y> <z> : scale part
//rotate <angle> <x> <y> <z> : rotate part by angle in degree form about pivot set in loadStructure()
//'.' for break, '#' for comment
bool Robot::loadScript(std::string script_path)
{
	this->script_src = script_path;
	std::ifstream ifs(script_path, std::ios_base::in);
	if (!ifs)
	{
		std::cout << exec << "Script file(\'" + script_path + "\' not found!" << std::endl;
		return false;
	}
	else
	{
		std::cout << exec << "Loading robot script(\'" << script_path << "\')" << std::endl;
	}

	actions.clear();
	std::string cmd, line, part_name, act_name;
	Part *ptr_part = nullptr;

	Script tmp_script;

	float x = 0, y = 0, z = 0, theta = 0, time = 0, time_tmp = 0;
	bool hasRot, hasSca, hasTra;
	hasRot = hasSca = hasTra = 0;
	int line_counter = 0;
	while (std::getline(ifs, line))
	{
		line_counter++;
		if (line.empty() || line.front() == '#') continue;
		if (line.front() == '.') break;
		std::stringstream ss(line);

		ss >> cmd;
		if (cmd == "act" && ss >> act_name)
		{
			if (!tmp_script.name.empty())
			{
				tmp_script.max_time = time;
				actions[tmp_script.name] = tmp_script;
			}

			tmp_script = Script();
			tmp_script.name = act_name;
			hasRot = hasSca = hasTra = 0;
		}
		else if (cmd == "usePart" && ss >> part_name)
		{
			auto it = ref_parts.find(part_name);
			if (it == ref_parts.end())
			{
				std::cout << '(' << script_path << ':' << line_counter
					<< ") Invalid part name: \'" << part_name << "\'\n";
				ptr_part = nullptr;
				continue;
			}
			ptr_part = it->second;
			hasRot = hasSca = hasTra = 0;
		}
		else if (cmd == "time" && ss >> time_tmp)
		{
			if (time_tmp < time)
			{
				std::cout << '(' << script_path << ':' << line_counter
					<< ") Timing must be in incremental order" << std::endl;
				return false;
			}
			time = time_tmp;
			ptr_part = nullptr;
			hasRot = hasSca = hasTra = 0;
		}
		else if (ptr_part == nullptr)
		{
			std::cout << '(' << script_path << ':' << line_counter
				<< ") Please use a part using \"usePart <part_id>\" first!\n";
		}
		else if (cmd == "rotate" && ss >> theta >> x >> y >> z)
		{
			Script::Keyframe key;
			key.time = time; 
			key.rot = angleAxis(radians(theta), vec3(x, y, z));
			if (!hasRot)
				tmp_script.effects[ptr_part][Script::ROTATE].push_back(key);
			else
				tmp_script.effects[ptr_part][Script::ROTATE].back().rot *= key.rot;
			hasRot = 1;
		}
		else if (cmd == "translate" && ss >> x >> y >> z)
		{
			Script::Keyframe key;
			key.time = time; 
			key.xyz = vec3(x, y, z);
			if (!hasTra)
				tmp_script.effects[ptr_part][Script::TRANSLATE].push_back(key);
			else
				tmp_script.effects[ptr_part][Script::TRANSLATE].back().xyz += key.xyz;
		}
		else if (cmd == "scale" && ss >> x >> y >> z)
		{
			Script::Keyframe key;
			key.time = time;
			key.xyz = vec3(x, y, z);
			if (!hasSca)
				tmp_script.effects[ptr_part][Script::SCALE].push_back(key);
			else
				tmp_script.effects[ptr_part][Script::SCALE].back().xyz *= key.xyz;
		}
		else
		{
			std::cout << '(' << script_path << ':' << line_counter
				<< ") Command: \'" + line + "\' invalid!\n";
		}
	}
	if (!tmp_script.name.empty())
	{
		tmp_script.max_time = time;
		actions[tmp_script.name] = tmp_script;
	}


	std::cout << exec << "Loaded robot script(\'" << script_path << "\')\n" << std::endl;
	return true;
}