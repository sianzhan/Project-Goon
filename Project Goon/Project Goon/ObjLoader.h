#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>

using namespace glm;

struct Material
{
	std::string name;
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
	unsigned int tex_Kd = 0;
	std::string map_Kd;
};

struct MtlInfo
{
	std::string material_name; //material name
	unsigned int count; //no of faces using this material.
	MtlInfo(std::string mn, unsigned int c): count(c), material_name(mn) {}
};


//output will be appended to the vector provided, in face-basis(face by face)
bool loadObj(
	const std::string obj_path,
	std::vector<vec3> & out_vertices,
	std::vector<vec2> & out_uvs,
	std::vector<vec3> & out_normals,
	std::vector<MtlInfo> &out_mtls
);

bool loadMtl(
	const std::string mtl_path,
	std::map <std::string, Material > &out_mtls
);