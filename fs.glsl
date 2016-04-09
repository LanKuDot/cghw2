#version 330

// Default color buffer location is 0
// If you create framebuffer your own, you need to take care of it
layout(location=0) out vec4 color;

in vec2 fTexcoord;
in vec4 worldPosition;
in vec4 worldNormal;

uniform sampler2D uSampler;

// Matiral and Light color
uniform vec4 sunPosition;	// Where is the SUN?
uniform vec4 sunLightColor;	// What is the color of the sunlight?
uniform vec4 planetAmbient;
uniform vec4 planetDiffuse;
uniform vec4 planetEmission;

void main()
{
	// Calculate the diffuse light
	vec4 meshNormal = normalize(worldNormal);
	vec4 shootToTheLight = normalize(sunPosition - worldPosition);
	// If the dot product of meshNormal and shootToTheLight is negative,
	// which means the mesh is away from the light, no need to do the diffuse reflection.
	vec4 diffuse = max(dot(meshNormal, shootToTheLight), 0) * sunLightColor * planetDiffuse;

	color = (planetEmission + diffuse + planetAmbient) * texture(uSampler,fTexcoord);
}
