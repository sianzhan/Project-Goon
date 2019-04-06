#ifdef GL_ES
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

vec4 permute(vec4 x) { return mod(((x*34.0)+1.0)*x, 289.0); }

vec2 fade(vec2 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}
float noise(vec2 P){
	vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
	vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
	Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
	vec4 ix = Pi.xzxz;
	vec4 iy = Pi.yyww;
	vec4 fx = Pf.xzxz;
	vec4 fy = Pf.yyww;
	vec4 i = permute(permute(ix) + iy);
	vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
	vec4 gy = abs(gx) - 0.5;
	vec4 tx = floor(gx + 0.5);
	gx = gx - tx;
	vec2 g00 = vec2(gx.x,gy.x);
	vec2 g10 = vec2(gx.y,gy.y);
	vec2 g01 = vec2(gx.z,gy.z);
	vec2 g11 = vec2(gx.w,gy.w);
	vec4 norm = 1.79284291400159 - 0.85373472095314 * 
	vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
	g00 *= norm.x;
	g01 *= norm.y;
	g10 *= norm.z;
	g11 *= norm.w;
	float n00 = dot(g00, vec2(fx.x, fy.x));
	float n10 = dot(g10, vec2(fx.y, fy.y));
	float n01 = dot(g01, vec2(fx.z, fy.z));
	float n11 = dot(g11, vec2(fx.w, fy.w));
	vec2 fade_xy = fade(Pf.xy);
	vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
	float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
	return 2.3 * n_xy;
}

float octave(vec2 P, int octaves, float persistence)
{
	float total = 0.0;
	float frequency = 1.0;
	float amplitude = 1.0;
	float max_value = 0.0;
for (int i = 0; i < 8; i++)
	{
		total += noise(P * frequency) * amplitude;
		max_value += amplitude;
		amplitude *= persistence;
		frequency *= 2.0;
	}

	return total / max_value;
}

void main( void ) {
	vec2 position = gl_FragCoord.xy / resolution.xy;
	position.x *= resolution.x / resolution.y;

	float l0 = pow(octave(vec2(position.x + time * 0.10, 0.0), 8, 0.5) * 0.5 + 0.1, 1.0);
	float f0 = pow(octave(vec2(position.x + time * 0.095, position.y + time * 0.01), 8, 0.75) * 0.1 + 0.1, 1.0);
	float l1 = pow(octave(vec2(position.x + time * 0.09, 0.0), 8, 0.5) * 0.5 + 0.1, 1.0);
	float l2 = pow(octave(vec2(position.x + time * 0.08, 0.0), 8, 0.5) * 0.5 + 0.2, 1.0);
	float l3 = pow(octave(vec2(position.x + time * 0.07, 0.0), 8, 0.5) * 0.5 + 0.2, 1.0);
	float l4 = pow(octave(vec2(position.x + time * 0.06, 0.0), 8, 0.5) * 0.5 + 0.3, 1.0);
	float l5 = pow(octave(vec2(position.x + time * 0.05, 0.0), 8, 0.5) * 0.5 + 0.3, 1.0);
	float l6 = pow(octave(vec2(position.x + time * 0.04, 0.0), 8, 0.5) * 0.5 + 0.4, 1.0);
	float l7 = pow(octave(vec2(position.x + time * 0.03, 0.0), 8, 0.5) * 0.5 + 0.4, 1.0);
	float l8 = pow(octave(vec2(position.x + time * 0.02, 0.0), 8, 0.5) * 0.5 + 0.5, 1.0);
	float l9 = pow(octave(vec2(position.x + time * 0.01, 0.0), 8, 0.5) * 0.5 + 0.5, 1.0);
	float f9 = pow(octave(vec2(position.x + time * 0.005, position.y + time * 0.001), 8, 0.75) * 0.5 + 0.75, 1.0);

	gl_FragColor = vec4(0.9, 0.8, 0.5, 1.0);
  vec2 pos = (gl_FragCoord.xy * 2.0 - resolution) / min(resolution.x, resolution.y);

  float v = 0.0;
  for (int i = 0; i < 200; i++) {
    float s = time + float(i) * 0.0075;
    vec2 mpos = 0.5 * vec2(sin(s * float(i) / 200.0), - cos(s * 3.0));
    float t = 0.01 / length(mpos - pos);
    v += pow(t, 2.0) * float(i + 1) / 100.0;
  }

  gl_FragColor = 1.0 * vec4(vec3(v)*vec3(0.5,0.75,1.0), 1.0);
	if (position.y < f9) { gl_FragColor += vec4(1.0) * (1.0 - f9); }
	if (position.y < l9) { gl_FragColor = vec4(l9, 1.0, 0.0, 1.0); }
	if (position.y < l8) { gl_FragColor = vec4(l8, 0.9, 0.0, 1.0); }
	if (position.y < l7) { gl_FragColor = vec4(l7, 0.8, 0.0, 1.0); }
	if (position.y < l6) { gl_FragColor = vec4(l6, 0.7, 0.0, 1.0); }
	if (position.y < l5) { gl_FragColor = vec4(l5, 0.6, 0.0, 1.0); }
	if (position.y < l4) { gl_FragColor = vec4(l4, 0.5, 0.0, 1.0); }
	if (position.y < l3) { gl_FragColor = vec4(l3, 0.4, 0.0, 1.0); }
	if (position.y < l2) { gl_FragColor = vec4(l2, 0.3, 0.0, 1.0); }
	if (position.y < l1) { gl_FragColor = vec4(l1, 0.2, 0.0, 1.0); } 
	if (position.y < f0) { gl_FragColor += vec4(1.0) * f0; }
	if (position.y < l0) { gl_FragColor = vec4(l0, 0.1, 0.0, 1.0); } 

}