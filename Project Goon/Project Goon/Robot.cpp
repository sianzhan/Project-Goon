#include "Robot.h"
#include "main.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
//Construct Robot and load it into buffer
#define PI 3.14159f

#define BINDING_POINT_MVP 0

std::string Robot::exec = "Robot: ";
const mat4 Robot::identity4 = mat4(1.0);

const Robot::Script::Effect Robot::effect_types[] = { Script::SCALE, Script::TRANSLATE, Script::ROTATE };

void Robot::init(const char* robot_path)
{
	this->destroy();

	//Initiate VAO and Buffers
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo_vertices);
	glGenBuffers(1, &vbo_uvs);
	glGenBuffers(1, &vbo_normals);
	glGenBuffers(1, &ubo_mvp);

	//put View & Projection matrix into Uniform Buffers
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_mvp);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * 5, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	//Load Parts informations from file
	loadRobotInfo(robot_path);
	loadPart2Buffer();
	try {
		loadShaders(shader_src);
	}
	catch (std::exception ex) { printf("%s\n", ex.what()); }
	if (programs.size() == 0)
	{
		std::cout << exec << "Error intializating robot: No valid shader programs!" << std::endl;
		return;
	}
	updateProgram();
	if (!struct_src.empty()) loadStructure(struct_src);
	if (!script_src.empty()) loadScript(script_src);

}

void Robot::act(std::string act_name)
{
	auto a_it = actions.find(act_name);
	if (a_it != actions.end())
	{
		action = &a_it->second;
		preload();
	}
	else action = nullptr;
}

void Robot::preload()
{
	for (Part &part : parts) {
		for (int i = 0; i < 2; ++i)
		{
			part.t_Tra[i] = vec3(0);
			part.t_Sca[i] = vec3(1.0f);
			part.t_Rot[i] = quat(1, 0, 0, 0);
		}
		for (Script::Effect effect_type : effect_types)
		{
			auto &effect_indices = part.effects_indices[effect_type];
			effect_indices[0] = effect_indices[1] = -1;
			part.time_offsets[effect_type] = 0;
		}
		if (action != nullptr)
		{
			auto effects = action->effects[&part];
			for (auto e_it = effects.begin(); e_it != effects.end(); e_it++)
			{
				Script::Effect effect_type = e_it->first;
				if (!e_it->second.empty())
				{
					Script::Keyframe keyframe = (e_it->second)[0];
					if (effect_type == Script::ROTATE)
					{
						part.t_Rot[1] = keyframe.rot;
					}
					else if (effect_type == Script::SCALE)
					{
						part.t_Sca[1] = keyframe.xyz;
					}
					else if (effect_type == Script::TRANSLATE)
					{
						part.t_Tra[1] = keyframe.xyz;
					}
				}
			}
		}
	}
	frame_count = 0;
}

void Robot::reload()
{
	std::cout << exec << "Reloading..." << std::endl;
	try {
		std::string act_name;
		for (Program &program : programs) //delete to load new shaders
			glDeleteProgram(program.data());
		programs.clear();
		active_program_idx = 0;
		action = nullptr;
		if (!shader_src.empty()) loadShaders(shader_src);
		if (!struct_src.empty()) loadStructure(struct_src);
		if (!script_src.empty()) loadScript(script_src);
		updateProgram();
	}
	catch (std::exception e)
	{
		printf("Reload Failed!: %s\n", e.what());
	}
}

void Robot::render(bool use_default)
{
	if (!use_default) { //for shadow depth map generation
		shadow_program.use();
		GLuint modelId = glGetUniformLocation(shadow_program.data(), "model");
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		for (int i = 0; i < parts.size(); i++)
		{
			GLuint offset_vbo = 0;
			glUniformMatrix4fv(modelId, 1, GL_FALSE, &parts[i].getModel()[0][0]);
			glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(offset_vbo * sizeof(vec3)));
			glDrawArrays(GL_TRIANGLES, offset_vbo, parts[i].count_vertices); //3 vertices per face due to triangulation
			offset_vbo += parts[i].count_vertices;
		}
		return;
	}
	programs[active_program_idx].use();
	glBindVertexArray(vao);
	//the second parameter is the index in ubb to bind to
	glUniform1f(uni_time_id, (float)glfwGetTime());
	glBindBufferRange(GL_UNIFORM_BUFFER, BINDING_POINT_MVP, ubo_mvp, 0, ubo_mvp_size);
	glUniformMatrix4fv(uni_mvp_depth_id, 1, GL_FALSE, &depthBiasMVP[0][0]);
	GLuint offset_vbo = 0;
	for (int i = 0; i < parts.size(); i++)
	{
		Part &part = parts[i];
		mat4 MV = View * part.getModel();
		mat4 MVP = Projection * MV;

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &part.getModel());
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), &View);
		glBufferSubData(GL_UNIFORM_BUFFER, 2*sizeof(mat4), sizeof(mat4), &Projection);
		glBufferSubData(GL_UNIFORM_BUFFER, 3*sizeof(mat4), sizeof(mat4), &MV);
		glBufferSubData(GL_UNIFORM_BUFFER, 4*sizeof(mat4), sizeof(mat4), &MVP);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(offset_vbo * sizeof(vec3)));

		glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(offset_vbo * sizeof(vec2)));

		glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)(offset_vbo * sizeof(vec3)));

		offset_vbo += part.count_vertices;

		GLuint offset_draw = 0;
		for (int k = 0; k < part.mtl_infos.size(); ++k)
		{
			MtlInfo &mtl_info = part.mtl_infos[k];
			Material &mtl = part.mtl[mtl_info.material_name];

			if (mtl.tex_Kd != 0)
			{
				glEnable(GL_TEXTURE_2D);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mtl.tex_Kd);
			}
			//in the order of Ka, Kd, Ks
			glUniform3fv(uni_mtl_id, 1, &mtl.Ka[0]);
			glUniform3fv(uni_mtl_id+1, 1, &mtl.Kd[0]);
			glUniform3fv(uni_mtl_id+2, 1, &mtl.Ks[0]); 

			glDrawArrays(GL_TRIANGLES, offset_draw, mtl_info.count * 3); //3 vertices per face due to triangulation
			offset_draw += mtl_info.count * 3;
			glDisable(GL_TEXTURE_2D);
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindVertexArray(0);
}
int lastDelta = 0;
void Robot::update()
{
	if (!action) return;
	float time = (frame_count += delta);
#ifdef TIMING 
	if (lastDelta > time) lastDelta = time;
	if (time - lastDelta > 1)
	{
		printf("%f\n", fmod(time,action->max_time)); 
		lastDelta = time;
	}
#endif
	auto p_it = action->effects.begin(); //Loop through all parts
	for (; p_it != action->effects.end(); ++p_it)
	{
		Part &part = *p_it->first;
		auto &effects = p_it->second;
		if (effects.size() == 0) continue;

		//Loop through all effects (SCALE, TRANSLATE, ROTATE)
		for (auto e_it = effects.begin(); e_it != effects.end(); e_it++)
		{
			auto &effect_type = e_it->first;
			auto &keyframes = e_it->second;
			if (keyframes.empty()) continue;
			auto &effect_idx = part.effects_indices[effect_type]; //effect idx
			auto &time_offset = part.time_offsets[effect_type]; //time offset for this effect

			if (effect_idx[0] >= (int)keyframes.size()) effect_idx[0] = 0;
			effect_idx[1] = (effect_idx[0] + 1) % keyframes.size();

			//end time of this effect
			const float &end_time = (effect_idx[0] == keyframes.size() - 1 && effect_idx[1] == 0 ?
				action->max_time + keyframes[0].time : keyframes[effect_idx[1]].time);
			if (time - time_offset > end_time) {
				effect_idx[0] = (effect_idx[0] + 1) % keyframes.size();
				effect_idx[1] = (effect_idx[0] + 1) % keyframes.size();
				//skil frame[n-1] -> frame[0] if their time is same
				if (effect_idx[0] == keyframes.size() - 1  && action->max_time
					- (keyframes[keyframes.size() - 1].time - keyframes[0].time) < 0.01)
				{
					effect_idx[0] = (effect_idx[0] + 1) % keyframes.size();
					effect_idx[1] = (effect_idx[0] + 1) % keyframes.size();
				}
				while (effect_idx[0] == 0 && time_offset + action->max_time <= time )
					time_offset += action->max_time;
				for (int i = 0; i < 2; ++i)
				{
					auto &keyframe = keyframes[effect_idx[i]];

					if (effect_type == Script::ROTATE)
					{
						part.t_Rot[i] = keyframe.rot;
					}
					else if (effect_type == Script::SCALE)
					{
						part.t_Sca[i] = keyframe.xyz;
					}
					else if (effect_type == Script::TRANSLATE)
					{
						part.t_Tra[i] = keyframe.xyz;
					}
				}
			}
		}
	}

	//Update Model for every part
	for (Part& part : parts)
	{
		part.Model = part.rawModel;
		auto p_it = action->effects.find(&part);
		if (p_it == action->effects.end()) continue;
		for (Script::Effect effect_type : effect_types)
		{
			auto e_it = p_it->second.find(effect_type);
			//if this effect doesn't exist in this part, skip
			if (e_it == p_it->second.end()) continue;
			const auto &keyframes = e_it->second;
			const auto &effect_idx = part.effects_indices[effect_type];
			const auto &time_offset = part.time_offsets[effect_type];
			//if this effect hasn't taken into effect, skip
			float end_time, tp;
			if (effect_idx[0] >= 0)
			{
				//Append the remaining time so that the shorter part can run in full cycle, with smooth transition end-to-front
				end_time = (effect_idx[0] == keyframes.size() - 1 && effect_idx[1] == 0 ?
					action->max_time + keyframes[0].time : keyframes[effect_idx[1]].time);
				//plus one to prevent divide by zero loooool
				//tp : Time Portion
				tp = (time - time_offset - keyframes[effect_idx[0]].time)
					/ (end_time - keyframes[effect_idx[0]].time);
				//printf("%f %f %f %f %f %s\n", time, time_offset, end_time, keyframes[effect_idx[0]].time, tp, part.name.c_str());
			}
			else
			{ //for the case where the robot just start, and idx = -1
				end_time = keyframes[0].time;
				tp = (time - time_offset) / (end_time);
			}

			
			if (effect_type == Script::TRANSLATE) {
				part.Model = translate(part.Model, part.t_Tra[0] * (1 - tp) + part.t_Tra[1] * tp);
			}
			//rotate & scaleat pivot
			else 
			{
				part.Model = translate(part.Model, part.pivotCoord);
				
				if (effect_type == Script::ROTATE)
					part.Model *= toMat4(slerp(part.t_Rot[0], part.t_Rot[1], tp));
				else if (effect_type == Script::SCALE)
					part.Model = scale(part.Model, part.t_Sca[0] * (1 - tp) + part.t_Sca[1] * tp);
				
				part.Model = translate(part.Model, -part.pivotCoord);
			}
		}
	}
}

const mat4 Robot::Part::getModel() const
{
	if(pivotPart != nullptr) return pivotPart->getModel() * Model;
	else return Model;
}

std::tuple<vec3, vec3, vec3> Robot::getHeadPos()
{
	if (head == nullptr) return std::make_tuple(vec3(), vec3(), vec3());
	return std::make_tuple(head->getModel() * vec4(eye_pos, 1.0f),
		head->getModel() * vec4(look_pos, 1.0f),
		head->getModel() * vec4(god_pos, 1.0f));
}

void Robot::destroy()
{
	glDeleteBuffers(1, &vbo_vertices);
	glDeleteBuffers(1, &vbo_uvs);
	glDeleteBuffers(1, &vbo_normals);
	glDeleteBuffers(1, &ubo_mvp);
	glDeleteVertexArrays(1, &vao);
	for (Program &program : programs) glDeleteProgram(program.data());
}

Robot::~Robot()
{
	this->destroy();
}

void Robot::updateProgram()
{
	Program &program = programs[active_program_idx];
	uni_mtl_id = glGetUniformLocation(program.data(), "mtl.Ka");
	uni_time_id = glGetUniformLocation(program.data(), "time");
	unb_mvp_id = glGetUniformBlockIndex(program.data(), "mat_MVP"); //bound to ubo_mvp
	uni_mvp_depth_id = glGetUniformLocation(program.data(), "mat_MVP_depth"); //bound to ubo_mvp
	//Get Uniform Block Size
	glGetActiveUniformBlockiv(program.data(), unb_mvp_id, GL_UNIFORM_BLOCK_DATA_SIZE, &ubo_mvp_size);
	//bind UBB to binding point of mvp
	glUniformBlockBinding(program.data(), unb_mvp_id, BINDING_POINT_MVP);

}

void Robot::nextProgram()
{
	if (active_program_idx < programs.size() - 1)
	{
		++active_program_idx;
		updateProgram();
	}
}
void Robot::prevProgram()
{
	if (active_program_idx > 0)
	{
		--active_program_idx;
		updateProgram();
	}
}