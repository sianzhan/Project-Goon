#version 410 core


out vec4 out_color;

uniform sampler2D tex;

in VS_FS_VERTEX
{
	vec3 normal;
	vec2 tex_coord;
	vec3 lightdir;
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

float rt_w = 600;
float rt_h = 400;
float stitching_size = 100 + 10 * sin(time/70);
int invert = 1;

vec4 PostFX(sampler2D tex, vec2 uv, float time)
{
  vec4 c = vec4(0.0);
  float size = stitching_size;
  vec2 cPos = uv * vec2(rt_w, rt_h);
  vec2 tlPos = floor(cPos / vec2(size, size));
  tlPos *= size;
  int remX = int(mod(cPos.x, size));
  int remY = int(mod(cPos.y, size));
  if (remX == 0 && remY == 0)
    tlPos = cPos;
  vec2 blPos = tlPos;
  blPos.y += (size - 1.0);
  if ((remX == remY) || 
     (((int(cPos.x) - int(blPos.x)) == (int(blPos.y) - int(cPos.y)))))
  {
    if (invert == 1)
      c = vec4(33.5, 34.35, 39.25, 1.0);
    else
      c = texture(tex, tlPos * vec2(1.0/rt_w, 1.0/rt_h)) * 1.4;
  }
  else
  {
    if (invert == 1)
      c = texture(tex, tlPos * vec2(1.0/rt_w, 1.0/rt_h)) * 1.4;
    else
      c = vec4(0.0, 0.0, 0.0, 1.0);
  }
  return c;
}

void main (void)
{
    vec2 uv = vs_in.tex_coord;
    out_color = 0.6 * texture(tex, uv) + 0.3 * PostFX(tex, uv, time);
}