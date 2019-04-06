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
float stitching_size = 5.0 * sin(time/100) + 35;
int invert = 1;

vec4 PostFX(sampler2D tex, vec2 uv, double time)
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
      c = vec4(0.2, 0.15, 0.05, 1.0);
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
  float visibility = 1.0;
  if(textureProj(shadow_map, vs_in.shadow_coord) < gl_FragCoord.z) visibility = 0.5;
  out_color = vec4(visibility, 0,0,0);
  //out_color = vec4(texture(shadow_map, vs_in.shadow_coord.xyz), 1.0, 1.0, 1.0);
 /* /*
   vec2 uv = vs_in.tex_coord;
    out_color = 0.7 * PostFX(tex, uv, time) + 0.3 * texture(tex, uv);
    vec4 bla = vec4(depth, 0, 0 ,0);
    //out_color += bla ;
    out_color = ambientColor * mtl.Ka + diffuseColor * mtl.Kd + specularColor * mtl.Ks;
    //if(depth < vs_in.shadow_coord.z) out_color /= 2;*/
      float diff = max(0.0, dot(normalize(vs_in.normal), normalize(vs_in.lightdir)));
    if(length(vs_in.normal) < 0.01) diff = 0.8;
    // Multiply intensity by diffuse color, force alpha to 1.0
    vec4 tex_color = texture(tex, vs_in.tex_coord);
    out_color = visibility * diff * diffuseColor * vec4(mtl.Kd, 1) * tex_color *1.5;
    

    // Add in ambient light
   // out_color += vec4(vs_in.color.xyz,0)/10;
    out_color += ambientColor * tex_color * mtl.Ka;


    // Specular Light
    vec3 view_reflection = normalize(reflect(-normalize(vs_in.lightdir), normalize(vs_in.normal)));//反射角
    float spec = max(0.0, dot(normalize(vs_in.normal), view_reflection));
    if(diff != 0) {
        spec = pow(spec, shininess);
        out_color += visibility * specularColor*vec4(mtl.Ks,1)*spec*tex_color;

    }
}