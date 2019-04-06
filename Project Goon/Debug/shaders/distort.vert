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
	mat4 M;
	mat4 V;
	mat4 P;
	mat4 MV;
	mat4 MVP;
};

uniform mat4 mat_MVP_depth;
uniform float time;

vec2 Distort(vec2 p)
{
    float theta  = atan(p.y, p.x);
    float radius = length(p);
    radius = pow(radius, (sin(time*2)+10)/10);
    p.x = radius * cos(theta);
    p.y = radius * sin(theta);
    return 0.5 * (p + 1.0);
}

void main()
{
	// Get vertex position in view coordinates
    vec4 view_pos4 = MV * vec4(in_vertex.xyz, 1);
    vec3 view_pos3 = view_pos4.xyz / view_pos4.w;

    vertex.normal = mat3(MV) * in_normal.xyz;
    vertex.lightdir = normalize(view_lightpos - view_pos3);
    vertex.tex_coord = in_uv;


    //buddy shadow mapping, stilll working in progress
	//vertex.shadow_coord = mat_MVP_depth * M * vec4(in_vertex.xyz,1);

    vec4 model = M * vec4(in_vertex.xyz, 1);
    vec2 distort = Distort(model.xz);
	gl_Position = P * V * vec4(distort.x, model.y, distort.y, 1);
}
