#version 330
layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;

uniform mat4 model;	// Model matrix
uniform mat4 vp;	// View Projection matrix

// 'out' means vertex shader output for fragment shader
// fNormal will be interpolated before passing to fragment shader
out vec2 fTexcoord;
out vec4 worldPosition;
out vec4 worldNormal;

void main()
{
	fTexcoord = texcoord;

	worldPosition = model * vec4(position, 1.0f);
	worldNormal = model * vec4(normal, 0.0f);
	
	// Transfrom current vertex to clip-space position.
	gl_Position = vp * model * vec4(position, 1.0);
}
