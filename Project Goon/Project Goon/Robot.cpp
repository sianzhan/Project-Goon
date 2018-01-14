#include "Robot.h"
#include "main.h"
#include <glm/gtc/matrix_transform.hpp>
//Construct Robot and load it into buffer
#define PI 3.14159f

std::string Robot::exec = "Robot: ";
const mat4 Robot::identity4 = mat4(1.0);

void Robot::init(std::string robot_path)
{
	//Initiate VAO and Buffers
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo_vertices);
	glGenBuffers(1, &vbo_uvs);
	glGenBuffers(1, &vbo_normals);
	glGenBuffers(1, &ubo_mvp);
	uni_mtl_id = glGetUniformLocation(program.data(), "Material");
	unb_mvp_id = glGetUniformBlockIndex(program.data(), "mat_MVP"); //bound to ubo_mvp

	//Load Parts informations from file
	loadRobotInfo(robot_path);
	loadPart2Buffer();
	loadStructure("struct.txt");
	loadScript("script.txt");
	action =&actions["walk"];
	//printf("%d\n", action->keyframes.size());
}



void Robot::reload()
{
	std::cout << exec << "Reloading..." << std::endl;
	if (!struct_src.empty()) loadStructure(struct_src);
	if (!script_src.empty()) loadScript(script_src);
	for (Part &part : parts) part.frame[0] = -1;
}

float delta = 0;
void Robot::render()
{
	delta+=0.2;
	update(delta);
	glBindVertexArray(vao);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_mvp);
	float rad = 7;
	View = lookAt(
		glm::vec3(rad * sin(0/40.0), sin(0/107.0)+0.5, rad * cos(0/40.0)), // Camera is at (0,10,25), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,1,0 to look upside-down)
	);

	GLuint offset_vbo = 0;
	for (int i = 0; i < parts.size(); i++)
	{
		Part &part = parts[i];
		mat4 MV = View * part.getModel();
		mat4 MVP = Projection * MV;

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &MV);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), &MVP);

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

			glUniform3fv(uni_mtl_id, 3, &mtl.Ka[0]); //in the order of Ka, Kd, Ks
			glDrawArrays(GL_TRIANGLES, offset_draw, mtl_info.count * 3); //3 vertices per face due to triangulation
			offset_draw += mtl_info.count * 3;

			glDisable(GL_TEXTURE_2D);
		}
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindVertexArray(0);
}
void Robot::update(float time)
{
	if (!action) return;
	auto it = action->keyframes.begin();
	for (; it != action->keyframes.end(); ++it)
	{
		Part &part = *it->first;
		std::vector<Script::Keyframe> &keyframes = it->second;
		if (keyframes.size() == 0) continue;
		if (part.frame[0] >= (int)keyframes.size()) part.frame[0] = 0;
		part.frame[1] = (part.frame[0] + 1) % keyframes.size();
		
		//Append the remaining time so that the shorter part can run in full cycle, with smooth transition end-to-front
		const float &end_time = (part.frame[0] == keyframes.size()-1 && part.frame[1] == 0 ? 
			action->max_time + keyframes[0].time : keyframes[part.frame[1]].time);

		if (time - part.time_offset > end_time) {
			//printf("%d %d %f %f\n", part.frame[0], part.frame[1], time - part.time_offset, end_time);
			part.frame[0] = (part.frame[0] + 1) % keyframes.size();
			part.frame[1] = (part.frame[0] + 1) % keyframes.size();
			if (part.frame[0] == 0) part.time_offset = time - keyframes[0].time;

			//part.t_Tra[0] = part.t_Tra[1] = vec3(0);
			//part.t_Sca[0] = part.t_Sca[1] = vec3(1.0);
			//part.t_Rot[0] = part.t_Rot[1] = quat(1, 0, 0, 0);
			for (int i = 0; i < 2; ++i)
			{
				auto &effects = keyframes[part.frame[i]].effects;
				for (auto effect : effects)
				{
					if (effect.type == effect.ROTATE)
					{
						part.t_Rot[i] = quat(1, 0, 0, 0);
					}
					else if (effect.type == effect.SCALE)
					{
						part.t_Sca[i] = vec3(1.0);
					}
					else if (effect.type == effect.TRANSLATE)
					{
						part.t_Tra[i] = vec3(0);
					}
				}
				for (auto effect : effects)
				{
					if (effect.type == effect.ROTATE)
					{
						part.t_Rot[i] = angleAxis(radians(effect.theta), effect.xyz) * part.t_Rot[i];
					}
					else if (effect.type == effect.SCALE)
					{
						part.t_Sca[i] *= effect.xyz;
					}
					else if (effect.type == effect.TRANSLATE)
					{
						part.t_Tra[i] += effect.xyz;
					}
				}
			}
		}
	}
	for (Part& part : parts)
	{
		if (part.frame[0] < 0) continue;
		std::vector<Script::Keyframe> &keyframes = action->keyframes[&part];

		//Append the remaining time so that the shorter part can run in full cycle, with smooth transition end-to-front
		const float &end_time = (part.frame[0] == keyframes.size() - 1 && part.frame[1] == 0 ?
			action->max_time + keyframes[0].time : keyframes[part.frame[1]].time);
		
		//plus one to prevent divide by zero loooool
		//tp : Time Portion
		float tp = (time - part.time_offset - keyframes[part.frame[0]].time + 1)
			/ (end_time - keyframes[part.frame[0]].time + 1);
		
		part.Model = scale(part.rawModel, part.t_Sca[0]*(1-tp) + part.t_Sca[1]*tp);
		part.Model = translate(part.Model, part.t_Tra[0]*(1-tp) + part.t_Tra[1]*tp);
		//part.Model = (part.t_Model[1] * (time_portion)+part.t_Model[0] * (1 - time_portion));
		
		//rotate at pivot
		part.Model = translate(part.Model, part.pivotCoord);
		part.Model *= toMat4(slerp(part.t_Rot[0], part.t_Rot[1], tp));
		part.Model = translate(part.Model, -part.pivotCoord);

	}
}

const mat4 Robot::Part::getModel() const
{
	if(pivotPart != nullptr) return pivotPart->getModel() * Model;
	else return Model;
}