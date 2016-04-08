#version 330
layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;

uniform mat4 model;
uniform mat4 vp;
uniform float rotateDeg;

// 'out' means vertex shader output for fragment shader
// fNormal will be interpolated before passing to fragment shader
out vec2 fTexcoord;

void main()
{
	// Circular shift the texcoord
	float new_x = texcoord.x + rotateDeg / 360.0f;
	// No need to normalize the texcoord within [0,1] !?
	fTexcoord = vec2(new_x, texcoord.y);
	
	// Transfrom current vertex to clip-space position.
	gl_Position = vp * model * vec4(position, 1.0);
}
