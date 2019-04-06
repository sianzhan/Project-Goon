#version 410 core

//vertex shader
layout(location = 2) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 0) in vec3 in_vertex;

out VS_GS_VERTEX
{
 	vec3 normal;
 	vec2 tex_coord;
 	vec3 lightdir;
} vertex;

vec3 view_lightpos = vec3(0, 10,0);

uniform mat_MVP{
	mat4 MV;
	mat4 MVP;
};

void main()
{
	// Get vertex position in view coordinates
    vec4 view_pos4 = MV * vec4(in_vertex.xyz, 1);
    vec3 view_pos3 = view_pos4.xyz / view_pos4.w;

    vertex.normal = mat3(MV) * in_normal.xyz;
    vertex.lightdir = normalize(view_lightpos - view_pos3);
    vertex.tex_coord = in_uv;

	gl_Position = MVP * vec4(in_vertex, 1);
}
