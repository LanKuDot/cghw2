#version 330

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;

uniform mat4 model;	// Model matrix
uniform mat4 vp;	// View Projection matrix

out vec2 fTexcoord;
// For phong shader, interplote the vertex normal, and than pass to fs.
out vec4 worldNormal;
out vec4 worldPosition;

void main()
{
	fTexcoord = texcoord;
	worldNormal = normalize(model * vec4(normal, 0.0f));
	worldPosition = model * vec4(position, 1.0f);

	gl_Position = vp * worldPosition;
}
