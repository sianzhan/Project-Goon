#include "Robot.h"
#include "Texture/Texture.h"
#include "main.h"
#include <glm/gtc/matrix_transform.hpp>
//Construct Robot and load it into buffer
std::string Robot::exec = "Robot: ";
void Robot::init()
{
	parts.push_back(Part("UmbreonLowPoly.obj", "UmbreonLowPoly.mtl"));
	//parts.push_back(Part("2B.obj", "2B.mtl"));
	parts.push_back(Part("Churineout.obj", "Churineout.mtl"));

	std::vector<vec3> out_vertices;
	std::vector<vec2> out_uvs;
	std::vector<vec3> out_normals;

	for (int i = 0; i < parts.size(); ++i)
	{
		Part &part = parts[i];
		std::vector<MtlInfo> out_mtl_infos; //moved in so that it won't stack together

		//Load Mtl
		if (!part.mtl_source.empty()) loadMtl(part.mtl_source, part.mtl);

		//Load textures for material
		for (std::map<std::string, Material>::iterator it = part.mtl.begin();
			it != part.mtl.end(); it++)
		{
			Material &mtl = it->second;
			if (mtl.tex_Kd == 0 && !mtl.map_Kd.empty())
			{
				mtl.tex_Kd = Texture::GenTexture(mtl.map_Kd.c_str());
				if (mtl.tex_Kd) std::cout << exec << "Loaded Texture: " << mtl.map_Kd << std::endl;
			}
		}

		//Load Faces
		loadObj(part.obj_source, out_vertices, out_uvs, out_normals, out_mtl_infos);

		part.count_vertices = out_vertices.size();
		part.mtl_infos = out_mtl_infos;
		if (i > 0)
		{
			part.count_vertices -= parts[i - 1].count_vertices;
		}
	}
	uni_mtl_id = glGetUniformLocation(program.data(), "Material");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * out_vertices.size(), out_vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_uvs);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * out_uvs.size(), out_uvs.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_normals);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * out_normals.size(), out_normals.data(), GL_STATIC_DRAW);
	
	//put View & Projection matrix into Uniform Buffers
	glGenBuffers(1, &ubo_vp);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_vp);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * 3, NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &View);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), &Projection);
	mat4 VP = Projection * View;
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4)*2, sizeof(mat4), &VP);

	glGetUniformBlockIndex(unb_vp_id, "mat_VP");
	//get uniform struct size
	int ubo_size = 0;
	glGetActiveUniformBlockiv(program.data(), unb_vp_id, GL_UNIFORM_BLOCK_DATA_SIZE, &ubo_size);
	//bind UBO to its idx
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo_vp, 0, ubo_size);
	glUniformBlockBinding(program.data(), unb_vp_id, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}

int delta = 0;
void Robot::render()
{
	delta++;
	glBindVertexArray(vao);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_vp);
	View = lookAt(
		glm::vec3(7 * sin(delta/40.0), sin(delta/107.0)+0.5, 7 * cos(delta/40.0)), // Camera is at (0,10,25), in World Space
		glm::vec3(0, 00, 0), // and looks at the origin
		glm::vec3(0, -1, 0)  // Head is up (set to 0,1,0 to look upside-down)
	);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &View);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), &Projection);
	mat4 VP = Projection * View;
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4) * 2, sizeof(mat4), &VP);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	GLuint offset_vbo = 0;
	for (int i = 0; i < parts.size(); i++)
	{
		Part &part = parts[i];

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

			//printf("%d %s\n", k, mtl_info.material_name.c_str());
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
	glBindVertexArray(0);
}