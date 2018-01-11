#define _CRT_SECURE_NO_WARNINGS
#include "ObjLoader.h"

bool loadObj(
	const std::string obj_path,
	std::vector<vec3> & out_vertices,
	std::vector<vec2> & out_uvs,
	std::vector<vec3> & out_normals,
	std::vector<MtlInfo> &out_mtl_infos
)
{
	std::string exec = "ObjLoader: ";
	std::cout <<exec << "Loading Obj: \'" << obj_path << "\'" << std::endl;

	FILE * file = fopen(obj_path.c_str(), "r");
	if (!file) throw std::runtime_error(std::string("Unable to load obj from path:\'") + obj_path + '\'');

	char cmd[128];
	char buf[1024];

	//data read from obj(input)
	std::vector<vec3> in_vertices;
	std::vector<vec2> in_uvs;
	std::vector<vec3> in_normals;
	std::vector<std::vector<std::vector<int>>> in_faces;

	vec2 uv; vec3 vertex, normal; 
	int face_counter = 0;
	std::string mtl_name;

	std::vector<std::vector<int>> temp_face;
	// x: indexVertex ; y: indexUV ; z: indexNormal
	//[Ver]tex + [UV] + Norm[al]
	std::vector<int> indices(3, 0);
	while (~fscanf(file, "%s", cmd))
	{
		if (!strcmp(cmd, "v")) //Vertex
		{
			if (fscanf(file, "%f %f %f", &vertex.x, &vertex.y, &vertex.z) == 3)
				in_vertices.push_back(vertex);
			else throw std::runtime_error("Obj not supported! (vertices)");
		}
		else if (!strcmp(cmd, "vt")) //Texture Coordinate
		{
			if (fscanf(file, "%f %f", &uv.x, &uv.y) == 2) {
				in_uvs.push_back(uv);
			}
			else throw std::runtime_error("Obj not supported! (uvs)");
		}
		else if (!strcmp(cmd, "vn")) //Normal Coordinate
		{
			if (fscanf(file, "%f %f %f", &normal.x, &normal.y, &normal.z) == 3)
				in_normals.push_back(normal);
			else throw std::runtime_error("Obj not supported! (normals)");
		}
		else if (!strcmp(cmd, "f")) //Face Vertex/UV/Normal Indices
		{
			temp_face.clear();
			while (fscanf(file, "%d", &indices[0]) == 1)
			{
				fscanf(file, "/%d", &indices[1]);
				fscanf(file, "/%d", &indices[2]);
				temp_face.push_back(indices);
				for(int z = 0; z < 3; ++z) indices[z] = 0;
			}
			if (temp_face.size() < 3) throw std::runtime_error("ObjLoader: Something went wrong! (face)");
			in_faces.push_back(temp_face);
			face_counter += temp_face.size() - 2; //n vertices contribute to (n-2) triangle faces;
		}
		else if (!strcmp(cmd, "usemtl"))
		{
			if (face_counter > 0)
			{
				out_mtl_infos.push_back(MtlInfo(mtl_name, face_counter));
			}
			fscanf(file, "%s\n", buf);
			mtl_name = buf;
			face_counter = 0;
		}
		else fscanf(file, "%*[^\n]");
		cmd[0] = 0;
	}
	//push back the last active mtl
	if (face_counter > 0) out_mtl_infos.push_back(MtlInfo(mtl_name, face_counter));

	std::cout << exec << "Read " << in_vertices.size() << " vertices" << std::endl;
	std::cout << exec << "Read " << in_uvs.size() << " uvs" << std::endl;
	std::cout << exec << "Read " << in_normals.size() << " normals" << std::endl;

	/*for (int i = 0; i < in_faces.size(); ++i)
	{
		for (int j = 0; j < in_faces[i].size(); ++j) printf("%d/%d/%d ", in_faces[i][j][0], in_faces[i][j][1], in_faces[i][j][2]);
		printf("\n");
	}
	printf("END\n");*/
	for (int i = 0; i < in_faces.size(); ++i)
	{
		//Every face is composed of many vertices
		vec3 vertex_indices = { 0, 1, 2 };

		//Triangulate the Face, starts from 0,1,2 to 0,2,3 to ... 0,1+n,2+n
		for (; vertex_indices.z < in_faces[i].size(); ++vertex_indices.y, ++vertex_indices.z)
		{
			for (int k = 0; k < 3; ++k)
			{
				std::vector<int> &vertex_info = in_faces[i][vertex_indices[k]];

				//minus 1 because of the indices is 1-indexed.
				if (vertex_info[0] > 0)out_vertices.push_back(in_vertices[vertex_info[0]-1]);
				else out_vertices.push_back({ 0, 0 , 0 });
				if(vertex_info[1] > 0) out_uvs.push_back(in_uvs[vertex_info[1]-1]);
				else out_uvs.push_back({ 0, 0 });
				if (vertex_info[2] > 0) out_normals.push_back(in_normals[vertex_info[2]-1]);
				else out_normals.push_back({ 0, 0 , 0});
			}
		}
	}
	std::cout << exec << "Processed " << out_vertices.size() << " faces" << std::endl;
	std::cout <<exec<< "Loaded Obj: \'" << obj_path << "\'" << std::endl;
}


bool loadMtl(
	const std::string mtl_path,
	std::map<std::string, Material> &out_mtls
)
{
	std::string exec = "ObjLoader: ";
	std::cout << '\n' << exec << "Loading Mtl: \'" << mtl_path << "\'" << std::endl;

	std::ifstream ifs(mtl_path, std::ifstream::in);
	if (!ifs) throw std::runtime_error(std::string("Unable to load mtl from path:\'") + mtl_path + '\'');

	std::stringstream ss;
	char line[1000];
	std::string cmd;

	vec3 tmp;
	std::string buf, mtl_name;

	while (ifs.getline(line, 1000))
	{
		ss.str(line);
		if (ss.str().empty()) continue;
		ss.seekg(0);
		if (ss >> cmd)
		{
			if (cmd == "newmtl") //Vertex
			{
				if (ss >> buf) 
				{
					out_mtls[mtl_name].name = mtl_name  = buf;
				}
				else throw std::runtime_error("Incorrect mtl name!");
			}
			else if (cmd == "Kd") //Texture Coordinate
			{
				if (ss >> tmp.x >> tmp.y >> tmp.z)
					out_mtls[mtl_name].Kd = tmp;
				else throw std::runtime_error("Incorrect mtl Kd");
			}
			else if (cmd == "Ka") //Normal Coordinate
			{
				if (ss >> tmp.x >> tmp.y >> tmp.z)
					out_mtls[mtl_name].Ka = tmp;
				else throw std::runtime_error("Incorrect mtl Ka");
			}
			else if (cmd == "Ks") //Face Vertex/UV/Normal Indices
			{
				if (ss >> tmp.x >> tmp.y >> tmp.z)
					out_mtls[mtl_name].Ks = tmp;
				else throw std::runtime_error("Incorrect mtl Ks");
			}
			else if (cmd == "map_Kd")
			{
				if (ss >> buf)
					out_mtls[mtl_name].map_Kd = buf;
				else throw std::runtime_error("Incorrect mtl map_Kd!");
			}
		}
	}
	std::cout <<exec<< "Loaded Mtl: \'" + mtl_path << std::endl;

}