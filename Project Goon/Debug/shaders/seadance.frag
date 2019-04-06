#version 410 core

#define PI 3.141592653589793
#define HALF_PI 1.5707963267948966
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

/*
float viewAngle = 1.0;
float speed = 3.0;
float rate = 15.0;
float baseamp = 0.10;
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
  out_color = c*200;


}   
*/
float linear(float t) {
  return t;
}

float exponentialIn(float t) {
  return t == 0.0 ? t : pow(2.0, 10.0 * (t - 1.0));
}

float exponentialOut(float t) {
  return t == 1.0 ? t : 1.0 - pow(2.0, -10.0 * t);
}

float exponentialInOut(float t) {
  return t == 0.0 || t == 1.0
    ? t
    : t < 0.5
    ? +0.5 * pow(2.0, (20.0 * t) - 10.0)
    : -0.5 * pow(2.0, 10.0 - (t * 20.0)) + 1.0;
}

float sineIn(float t) {
  return sin((t - 1.0) * HALF_PI) + 1.0;
}

float sineOut(float t) {
  return sin(t * HALF_PI);
}

float sineInOut(float t) {
  return -0.5 * (cos(PI * t) - 1.0);
}

float qinticIn(float t) {
  return pow(t, 5.0);
}

float qinticOut(float t) {
  return 1.0 - (pow(t - 1.0, 5.0));
}

float qinticInOut(float t) {
  return t < 0.5
    ? +16.0 * pow(t, 5.0)
    : -0.5 * pow(2.0 * t - 2.0, 5.0) + 1.0;
}

float quarticIn(float t) {
  return pow(t, 4.0);
}

float quarticOut(float t) {
  return pow(t - 1.0, 3.0) * (1.0 - t) + 1.0;
}

float quarticInOut(float t) {
  return t < 0.5
  ? +8.0 * pow(t, 4.0)
  : -8.0 * pow(t - 1.0, 4.0) + 1.0;
}

float quadraticInOut(float t) {
  float p = 2.0 * t * t;
  return t < 0.5 ? p : -p + (4.0 * t) - 1.0;
}

float quadraticIn(float t) {
  return t * t;
}

float quadraticOut(float t) {
  return -t * (t - 2.0);
}

float cubicIn(float t) {
  return t * t * t;
}

float cubicOut(float t) {
  float f = t - 1.0;
  return f * f * f + 1.0;
}

float cubicInOut(float t) {
  return t < 0.5
  ? 4.0 * t * t * t
  : 0.5 * pow(2.0 * t - 2.0, 3.0) + 1.0;
}

float elasticIn(float t) {
  return sin(13.0 * t * HALF_PI) * pow(2.0, 10.0 * (t - 1.0));
}

float elasticOut(float t) {
  return sin(-13.0 * (t + 1.0) * HALF_PI) * pow(2.0, -10.0 * t) + 1.0;
}

float elasticInOut(float t) {
  return t < 0.5
  ? 0.5 * sin(+13.0 * HALF_PI * 2.0 * t) * pow(2.0, 10.0 * (2.0 * t - 1.0))
  : 0.5 * sin(-13.0 * HALF_PI * ((2.0 * t - 1.0) + 1.0)) * pow(2.0, -10.0 * (2.0 * t - 1.0)) + 1.0;
}

float circularIn(float t) {
  return 1.0 - sqrt(1.0 - t * t);
}

float circularOut(float t) {
  return sqrt((2.0 - t) * t);
}

float circularInOut(float t) {
  return t < 0.5
  ? 0.5 * (1.0 - sqrt(1.0 - 4.0 * t * t))
  : 0.5 * (sqrt((3.0 - 2.0 * t) * (2.0 * t - 1.0)) + 1.0);
}

float bounceOut(float t) {
  const float a = 4.0 / 11.0;
  const float b = 8.0 / 11.0;
  const float c = 9.0 / 10.0;
  
  const float ca = 4356.0 / 361.0;
  const float cb = 35442.0 / 1805.0;
  const float cc = 16061.0 / 1805.0;
  
  float t2 = t * t;
  
  return t < a
    ? 7.5625 * t2
    : t < b
    ? 9.075 * t2 - 9.9 * t + 3.4
    : t < c
    ? ca * t2 - cb * t + cc
    : 10.8 * t * t - 20.52 * t + 10.72;
}

float bounceIn(float t) {
  return 1.0 - bounceOut(1.0 - t);
}

float bounceInOut(float t) {
  return t < 0.5
  ? 0.5 * (1.0 - bounceOut(1.0 - t * 2.0))
  : 0.5 * bounceOut(t * 2.0 - 1.0) + 0.5;
}

float backIn(float t) {
  return pow(t, 3.0) - t * sin(t * PI);
}

float backOut(float t) {
  float f = 1.0 - t;
  return 1.0 - (pow(f, 3.0) - f * sin(f * PI));
}

float backInOut(float t) {
  float f = t < 0.5
  ? 2.0 * t
  : 1.0 - (2.0 * t - 1.0);
  
  float g = pow(f, 3.0) - f * sin(f * PI);
  
  return t < 0.5
  ? 0.5 * g
  : 0.5 * (1.0 - g) + 0.5;
}

mat2 rotate2d(float _angle){
  return mat2(cos(_angle), -sin(_angle),  sin(_angle), cos(_angle));
}

void main( void ) {

  vec2 uv = (vs_in.tex_coord * 2.0 - resolution.xy) / min(resolution.x, resolution.y);
  
  //uv *= rotate2d(time * 0.2);
  
  vec2 scaledUv = uv * 3000.0;
  vec2 repeatedUv = fract(scaledUv);
  repeatedUv -= 0.5;
  
  vec2 itemCenter = floor(scaledUv) + vec2(0.5, 0.5);
  float distanceFromCenter = distance(itemCenter, vec2(0.0));
  
  float t = fract(time * 0.8 + distanceFromCenter) * 2.0 - 1.0;
  float diameter = sineInOut(abs(t)-1.) ;
  float circle = (  length(uv*.1)+diameter );

  vec3 color = vec3(circle);
  color.b += fract(distanceFromCenter*sqrt(uv.x/uv.x)*PI*2.);
   
  out_color = vec4(color, 1.0)/4 + texture(tex, vs_in.tex_coord) * 1.0; 
}