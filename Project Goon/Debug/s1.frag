#version 410 core


out vec4 out_color;

uniform sampler2D tex;

in GS_FS_VERTEX
{
	vec3 normal;
	vec2 tex_coord;
	vec3 lightdir;
	vec4 color;
} geo_in; 

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
	float diff = max(0.0, dot(normalize(geo_in.normal), normalize(geo_in.lightdir)));

   if (diff > 0.8) {
        diff = 1.0;
    }
    else if (diff > 0.5) {
        diff = 0.6;
    }
    else if (diff > 0.2) {
        diff = 0.4;
    }
    else {
        diff = 0.2;
	}
    // Multiply intensity by diffuse color, force alpha to 1.0
    vec4 tex_color = texture(tex, geo_in.tex_coord);
    out_color = diff * diffuseColor * vec4(mtl.Kd, 1) * tex_color * 3;
	

	if(diff > 1.0) diff = 1.0;
    // Add in ambient light
   // out_color += vec4(geo_in.color.xyz,0)/10;
    out_color += diff* ambientColor * tex_color;


    // Specular Light
    vec3 view_reflection = normalize(reflect(-normalize(geo_in.lightdir), normalize(geo_in.normal)));//反射角
    float spec = max(0.0, dot(normalize(geo_in.normal), view_reflection));
    if(diff != 0) {
		spec = pow(spec, shininess);
		out_color += specularColor*vec4(mtl.Ks,1)*spec*tex_color;
    }

    //if(geo_in.dist_barycentric > 0.3) out_color = vec4(0, 0, 0, 0);
}