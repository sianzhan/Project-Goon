#version 410 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_GS_VERTEX
{
 	vec3 normal;
 	vec2 tex_coord;
 	vec3 lightdir;
} vertex_in[];

out GS_FS_VERTEX
{
	vec3 normal;
	vec2 tex_coord;
	vec3 lightdir;
	vec4 color;
} vertex_out;


void main(void)
{
	//out = in;
	int i;
	for(i = 0; i < gl_in.length(); ++i)
	{
		vertex_out.normal = vertex_in[i].normal;
		vertex_out.tex_coord = vertex_in[i].tex_coord;
		vertex_out.lightdir = vertex_in[i].lightdir;
		gl_Position = gl_in[i].gl_Position;
		vertex_out.color = gl_in[i].gl_Position;
		EmitVertex();

	}

}