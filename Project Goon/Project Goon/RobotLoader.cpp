#include "Robot.h"
#include "Texture/Texture.h"
#include "main.h"
#include <fstream>
#include <sstream>
#include <stack>
#include <glm/gtc/matrix_transform.hpp>

//Load Robot Informations (Part name, Part src, Material src)
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
	std::string line, name, obj_src, mtl_src;
	while (std::getline(ifs, line))
	{
		if (line.empty()) continue;
		std::stringstream ss(line);
		if (ss.peek() != '#') {
			ss >> name >> obj_src >> mtl_src;
			parts.push_back(Part(name, obj_src, mtl_src));
		}
	}
	std::cout << exec << "Loaded Robot Info from \'" << robot_path << "\'" << std::endl;
	std::cout << exec << "Crafting the robot...\n" << std::endl;

	//Put it down here instead of assigning it above because,
	//when vector performs resizing, the address changes, and thus BOOM.
	for (int i = 0; i < parts.size(); ++i) ref_parts[parts[i].name] = &parts[i];
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
			part.count_vertices -= parts[i - 1].count_vertices;
		}
		std::cout << std::endl;
	}

	
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * out_vertices.size(), out_vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * out_uvs.size(), out_uvs.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * out_normals.size(), out_normals.data(), GL_STATIC_DRAW);

	//put View & Projection matrix into Uniform Buffers
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_mvp);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * 2, NULL, GL_DYNAMIC_DRAW);

	//Get Uniform Block Size
	int unb_size = 0;
	glGetActiveUniformBlockiv(program.data(), unb_mvp_id, GL_UNIFORM_BLOCK_DATA_SIZE, &unb_size);
	//bind UBO to Uniform Block
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo_mvp, 0, unb_size);
	glUniformBlockBinding(program.data(), unb_mvp_id, 0);
	
	//Unbind Buffers
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}

//Pre: Robot Informations(Parts) must already been loaded by loadRobotInfo()
//Craft the robot by structuring & modelling the parts' pivot, location
//by manipulating their Model Matrix
bool Robot::loadStructure(std::string struct_path)
{
	this->struct_src = struct_path;
	for (int i = 0; i < parts.size(); ++i) parts[i].Model = identity4;
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
					<< ") Invalid part name: \'" << part_name << "\'" << std::endl;
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
				<< ") Please load a part using \"usePart <part_id>\" first!" << std::endl;
		}
		else if (cmd == "pushMatrix") //setup Pivot Part
		{
			stack_part.push(ptr_part);
		}
		else if (cmd == "popMatrix")
		{
			stack_part.pop();
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
		else if ((cmd == "break")) break;
		else
		{
			std::cout << '(' << struct_path << ':' << line_counter
				<< ") Command: \'" + line + "\' invalid!" << std::endl;
		}
	}
	if (ptr_Model) {
		*ptr_Model = translate(*ptr_Model, -pivot);
		*ptr_raw_Model = *ptr_Model; //update the value of rawModel to Model
	}
	std::cout << exec << "Loaded robot structure(\'" << struct_path << "\')\n" << std::endl;
	return true;
}

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

	std::string cmd, line, part_name, act;
	Part *ptr_part = nullptr;

	Script tmp_script;
	Script::Keyframe tmp_keyframe; 

	float x = 0, y = 0, z = 0, theta = 0, time = 0;
	int line_counter = 0;
	while (std::getline(ifs, line))
	{
		line_counter++;
		if (line.empty() || line.front() == '#') continue;
		std::stringstream ss(line);

		ss >> cmd;
		if (cmd == "act" && ss >> act)
		{
			if (!tmp_script.name.empty()) {
				if (tmp_keyframe.time >= 0 && ptr_part) {
					tmp_script.keyframes[ptr_part].push_back(tmp_keyframe);
				}
				tmp_script.max_time = time;
				actions[tmp_script.name] = tmp_script;
			}
			tmp_script = Script();
			tmp_script.name = act;

			tmp_keyframe.effects.clear();
			tmp_keyframe.time = -1; //Make it invalid
		}
		else if (cmd == "usePart" && ss >> part_name)
		{
			if (tmp_keyframe.time >= 0 && ptr_part)
				tmp_script.keyframes[ptr_part].push_back(tmp_keyframe);
			tmp_keyframe.effects.clear();

			auto it = ref_parts.find(part_name);
			if (it == ref_parts.end())
			{
				std::cout << '(' << script_path << ':' << line_counter
					<< ") Invalid part name: \'" << part_name << "\'" << std::endl;
				ptr_part = nullptr;
				continue;
			}
			ptr_part = it->second;
		}
		else if (cmd == "time" && ss >> time)
		{
			if (time < tmp_keyframe.time)
			{
				std::cout << '(' << script_path << ':' << line_counter
					<< ") Timing must be in incremental order" << std::endl;
				return false;
			}else if(tmp_keyframe.time >= 0 && ptr_part) 
				tmp_script.keyframes[ptr_part].push_back(tmp_keyframe);

			tmp_keyframe.effects.clear();
			tmp_keyframe.time = time;
			ptr_part = nullptr;
		}
		else if (ptr_part == nullptr)
		{
			std::cout << '(' << script_path << ':' << line_counter
				<< ") Please use a part using \"usePart <part_id>\" first!" << std::endl;
		}
		else if (cmd == "rotate" && ss >> theta >> x >> y >> z)
		{
			tmp_keyframe.effects.push_back(
				Script::Keyframe::Effect{ Script::Keyframe::Effect::ROTATE, vec3(x, y, z), theta });
		}
		else if (cmd == "translate" && ss >> x >> y >> z)
		{
			tmp_keyframe.effects.push_back(
				Script::Keyframe::Effect{ Script::Keyframe::Effect::TRANSLATE, vec3(x, y, z) });
		}
		else if (cmd == "scale" && ss >> x >> y >> z)
		{
			tmp_keyframe.effects.push_back(
				Script::Keyframe::Effect{ Script::Keyframe::Effect::SCALE, vec3(x, y, z) });
		}
		else if ((cmd == "break")) break;
		else
		{
			std::cout << '(' << script_path << ':' << line_counter
				<< ") Command: \'" + line + "\' invalid!" << std::endl;
		}
	}
	if (!tmp_script.name.empty()) {
		if (tmp_keyframe.time >= 0 && ptr_part) {
			tmp_script.keyframes[ptr_part].push_back(tmp_keyframe);
		}
		tmp_script.max_time = time;
		actions[tmp_script.name] = tmp_script;
	} //Assign the last keyframe

	std::cout << exec << "Loaded robot script(\'" << script_path << "\')\n" << std::endl;
	return true;
}