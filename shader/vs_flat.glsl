#version 330

#define AMBIENT 0
#define DIFFUSE 1
#define SPECULAR 2

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;

uniform mat4 model;	// Model matrix
uniform mat4 vp;	// View Projection matrix

uniform vec4 lightPosition;
uniform vec4 light[3];
uniform vec4 k[3];	// The refelection factor of material

out vec2 fTexcoord;
flat out vec4 phongColor;	// Do not interploation the output color.

void main()
{
	fTexcoord = texcoord;

	// The information which phong reflection model needed.
	vec4 worldPosition = model * vec4(position, 1.0f);
	vec4 worldNormal = normalize(model * vec4(normal, 0.0f));
	vec4 lightVector = normalize(lightPosition - worldPosition);
	// Phong reflection model
	vec4 diffuse = max(dot(worldNormal, lightVector), 0) * light[DIFFUSE] * k[DIFFUSE];
	phongColor = diffuse + light[AMBIENT] * k[AMBIENT];

	gl_Position = vp * worldPosition;
}
