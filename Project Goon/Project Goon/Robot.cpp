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
	frame[0] = -1;
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
		mat4 MV = View * (*part.pivotModel)* part.Model;
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
	static float time_offset = 0;
	if (!action) return;
	std::vector<Script::Keyframe> &keyframes = action->keyframes;

	if (frame[0] >= (int)keyframes.size()) frame[0] = 0;
	frame[1] = (frame[0] + 1) % keyframes.size();
  	if (time - time_offset > keyframes[frame[1]].time) {
		frame[0] = (frame[0] + 1) % keyframes.size();
		frame[1] = (frame[0] + 1) % keyframes.size();
		if (frame[0] == 0) time_offset = time;
		for (Part &part : parts)
		{
			part.t_Model[0] = part.t_Model[1] = part.rawModel;
			part.t_Rot[0] = part.t_Rot[1] = quat(1, 0, 0, 0);
		}

		for (int i = 0; i < 2; ++i)
		{
			auto it = keyframes[frame[i]].effects.begin();
			for (; it != keyframes[frame[i]].effects.end(); ++it)
			{
				Part &part = *it->first;
				auto &effects = it->second;
				for (auto effect : effects)
				{
					if (effect.type == effect.ROTATE)
					{
						part.t_Rot[i] = angleAxis(radians(effect.theta), effect.xyz) * part.t_Rot[i];
						//part.t_Model[i] = rotate(part.rawModel, radians(effect.theta), effect.xyz);
					}
					else if (effect.type == effect.SCALE)
					{
						part.t_Model[i] = scale(part.t_Model[i], effect.xyz);
					}
					else if (effect.type == effect.TRANSLATE)
					{
						part.t_Model[i] = translate(part.t_Model[i], effect.xyz);
					}
				}
			}
		}
	}
	float time_portion = (time - time_offset - keyframes[frame[0]].time + 1) 
		/ (keyframes[frame[1]].time - keyframes[frame[0]].time + 1);
	for (Part &part : parts)
	{
		//Part &part = parts[0];
		//part.Model = mix(part.t_Model[0], part.t_Model[1], time_portion);
		part.Model = (part.t_Model[1] * (time_portion)+part.t_Model[0] * (1 - time_portion));
		part.Model *= toMat4(slerp(part.t_Rot[0], part.t_Rot[1], time_portion));
	}
}