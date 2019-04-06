#version 440 core

//vertex shader
layout(location = 2) in vec4 offset;
layout(location = 1) in vec4 color;
layout(location = 0) in vec4 vertex;
out vec4 outcolor;
uniform sampler2D tex;

layout (location = 3) in vec2 in_tex_coord;
out vec2 tex_coord; 
void main()
{
	tex_coord = in_tex_coord;
	gl_Position = vec4(vertex.xyz * 2, 1) + offset;
	outcolor = color;
}
