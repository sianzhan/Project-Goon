#version 410 core


out vec4 out_color;

uniform sampler2D tex;

in GS_FS_VERTEX
{
	vec3 normal;
	vec2 tex_coord;
	vec3 lightdir;
	vec4 color;
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

void main(void)
{

    float diff = max(0.0, dot(normalize(vs_in.normal), normalize(vs_in.lightdir)));
    if(length(vs_in.normal) < 0.01) diff = 0.7;
    // Multiply intensity by diffuse color, force alpha to 1.0
    vec4 tex_color = texture(tex, vs_in.tex_coord);
    out_color = diff * diffuseColor * vec4(mtl.Kd, 1) * tex_color *1.5;
    

    // Add in ambient light
   // out_color += vec4(vs_in.color.xyz,0)/10;
    out_color += ambientColor * tex_color * mtl.Ka;


    // Specular Light
    vec3 view_reflection = normalize(reflect(-normalize(vs_in.lightdir), normalize(vs_in.normal)));//反射角
    float spec = max(0.0, dot(normalize(vs_in.normal), view_reflection));
    if(diff != 0) {
        spec = pow(spec, shininess);
        out_color += specularColor*vec4(mtl.Ks,1)*spec*tex_color;

    }
  /*  float ndcDepth = (2.0 * out_color.z - gl_DepthRange.near - gl_DepthRange.far) / (gl_DepthRange.far - gl_DepthRange.near);
    float clipDepth = ndcDepth / out_color.w;
    out_color = vec4((clipDepth * 0.5) + 0.5); */

}