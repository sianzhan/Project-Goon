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

uniform vec2 resolution = vec2(800, 600);
//lighting color
vec4    ambientColor = vec4(0.5,0.5,0.5,1);
vec4    diffuseColor = vec4(0.8,0.8,0.8,1);   
vec4    specularColor = vec4(1,1,1,1);
float shininess = 128.0;//for material specular

uniform Material mtl;
uniform float time;


uniform float luminanceThreshold = 0.2; // 0.2
uniform float colorAmplification = 4.0; // 4.0
uniform float effectCoverage = 2.0; // 0.5
void main ()
{
  vec4 finalColor;
  // Set effectCoverage to 1.0 for normal use.  
  if (vs_in.tex_coord.x < effectCoverage) 
  {
    vec2 uv;           
    uv.x = 0.4*sin(time*50.0);                                 
    uv.y = 0.4*cos(time*50.0);                                 
    float m = 1;
    vec3 n = vec3(1);//texture2D(noiseTex,              (vs_in.tex_coord.st*3.5) + uv).rgb;
    vec3 c = texture2D(tex, vs_in.tex_coord 
                               + (n.xy*0.005)).rgb;
  //out_color.rgb = c;
    float lum = dot(vec3(0.30, 0.59, 0.11), c);
    if (lum < luminanceThreshold)
      c *= colorAmplification; 
  
    vec3 visionColor = vec3(0.1, 0.95, 0.2);
    finalColor.rgb = (c + (n*0.2)) * visionColor * m;
   }
   else
   {
    finalColor = texture(tex, 
                   vs_in.tex_coord);
   }
   out_color = finalColor;
 // gl_FragColor.rgb = finalColor.rgb;
 // gl_FragColor.a = 1.0;
} 