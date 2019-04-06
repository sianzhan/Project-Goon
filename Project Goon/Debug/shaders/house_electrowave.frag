#version 410 core


out vec4 out_color;

uniform sampler2D tex;
uniform sampler2DShadow shadow_map;
in VS_FS_VERTEX
{
	vec3 normal;
	vec2 tex_coord;
	vec3 lightdir;
  vec4 shadow_coord;
} vs_in; 

struct Material{
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
};

//lighting color
vec4    ambientColor = vec4(0.5,0.5,0.5,1);
vec4    diffuseColor = vec4(0.8,0.8,0.8,1);   
vec4    specularColor = vec4(1,1,1,1);
float shininess = 128.0;//for material specular

uniform Material mtl;
uniform float time;


float viewAngle = 2.0;
float speed = 1.0;
float rate = 15.0;
float baseamp = 0.50;
void main(void)
{
  vec2 p = -1.0 + 2.0 * vs_in.tex_coord.xy;
  float x = speed * viewAngle * time + rate * p.x;
  float base = (1.0 + cos(x*2.5 + time)) * (1.0 + sin(x*3.5 + time));
  float z = fract(0.05*x);
  z = max(z, 1.0-z);
  z = pow(z, 50.0);
  float pulse = exp(-10000.0 * z);
  vec4 ecg_color = vec4(0.6, 0.7, 0.8, 1.0);
  vec4 c = pow(clamp(1.0-abs(p.y-(baseamp*base+pulse-0.5)), 0.0, 1.0), 16.0) * ecg_color;
  out_color = c + 0.2*texture(tex, vs_in.tex_coord);
}
