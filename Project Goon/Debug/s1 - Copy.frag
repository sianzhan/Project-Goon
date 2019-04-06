#version 440 core

in vec4 outcolor;
in vec2 tex_coord;
out vec4 color2;
uniform sampler2D tex;
void main(void)
{
	vec4 color = texture(tex, tex_coord);
	float gray = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b; 
	color =  vec4(vec3(gray), 1.0) ;//+ 0.8 * color;
	int nbins = 3;
	float r = floor(color.r * float(nbins)) / float(nbins);
	float g = floor(color.g * float(nbins)) / float(nbins);
	float b = floor(color.b * float(nbins)) / float(nbins);
	color2 = vec4(r,g,b,color.a);
}