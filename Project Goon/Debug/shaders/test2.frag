#version 410 core

out vec4 out_color;
uniform sampler2D tex;
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

uniform float time;
uniform vec2 mouse = vec2(1, 1);
uniform vec2 resolution = vec2(800, 600);

float rand(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 offset(ivec2 tile) {
	float r = rand(vec2(tile * 10));
	if (r < 5.0 / 15.0) return vec2(0.0);
	if (r < 10.0 / 15.0) return vec2(-1.0);
	if (r < 14.0 / 15.0) return vec2(1.0);
	return vec2(-0.5);
}

const vec3 orange = vec3(0.85, 0.35, 0.3);
const vec3 blue = vec3(0.12, 0.2, 0.3);
const vec3 ecru = vec3(0.8, 0.75, 0.68);
const vec3 dark = vec3(0.02, 0.02, 0.06);
const vec3 dark_blue = vec3(0.08, 0.1, 0.15);
const vec3 light = vec3(0.85, 0.85, 0.84);

vec3 rand_color (vec2 tile) {
	float r = rand(tile);
	if (r < 1.0 / 6.0) return orange;
	if (r < 2.0 / 6.0) return blue;
	if (r < 3.0 / 6.0) return ecru;
	if (r < 4.0 / 6.0) return dark_blue;
	if (r < 5.0 / 6.0) return dark;
	return light;
}

const float dimx = 16.0;
const float dimy = 9.0;

void main( void ) {

	vec2 position = vs_in.tex_coord * sin(time/100);
	ivec2 tile = ivec2(int(position.x * dimx), int(position.y * dimy));
	vec2 subpos = vec2(dimx * position.x - float(tile.x), dimy * position.y - float(tile.y));

	vec3 a = rand_color(2.34 * vec2(tile) + 5.2);
	vec3 b = rand_color(vec2(tile) * 2.78 + 7.8);
	
	vec3 k = mix(rand_color(16.5 * vec2(tile)), rand_color(9.5 * vec2(tile)), vec3(0.5 + 0.5 * sin(time)));
	vec3 u = mix(rand_color(2.4 * vec2(tile)), rand_color(5.8 * vec2(tile)), vec3(0.5 + 0.5 * cos(time)));
	
	float diff = length(subpos + sin(0.8 * time) * offset(tile) + 1.5 * cos(0.3 * time + 42.0 * vec2(tile)));
	vec3 value = diff > 1.0 ? k : u;
	
	gl_FragColor = vec4( vec3(value), 1.0 ) + 0.2 * texture(tex, vs_in.tex_coord);

}