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

float rt_w = 800;
float rt_h = 600;

vec3 tc = vec3(1.0, 0.0, 0.0);
uniform float pixel_w = 35.0; // 15.0
uniform float pixel_h = 35.0; // 10.0
uniform float vx_offset = 2;
void main (void)
{
    vec2 uv = vs_in.tex_coord;
    if (uv.x < (vx_offset-0.005))
    {
        float dx = pixel_w*(1./rt_w);
        float dy = pixel_h*(1./rt_h);
        vec2 coord = vec2(dx*floor(uv.x/dx),
                          dy*floor(uv.y/dy));
        tc = texture(tex, coord).rgb;
    }
    else if (uv.x>=(vx_offset+0.005))
    {
        tc = texture(tex, uv).rgb;
    }

    float diff = max(0.0, dot(normalize(vs_in.normal), normalize(vs_in.lightdir)));
    if(length(vs_in.normal) < 0.01) diff = 0.8;
    // Multiply intensity by diffuse color, force alpha to 1.0

    out_color = diffuseColor * vec4(mtl.Kd, 1) * vec4(tc, 1) * 1.3;    

}