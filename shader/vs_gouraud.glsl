#version 330

#define AMBIENT 0
#define DIFFUSE 1
#define SPECULAR 2

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;

uniform mat4 model;	// Model matrix
uniform mat4 vp;	// View Projection matrix

uniform vec4 viewPosition;
uniform vec4 lightPosition;
uniform vec4 light[3];
uniform vec4 k[3];	// The refelection factor of material
uniform float shininess;
uniform float d_factor[3];	// The factor of distance quadratic funtion.

out vec2 fTexcoord;
// For the gouraud shader, interplote the vectex color.
out vec4 phongColor;

void main()
{
	fTexcoord = texcoord;

	/* The information which phong reflection model needed. */
	vec4 worldPosition = model * vec4(position, 1.0f);
	vec4 worldNormal = normalize(model * vec4(normal, 0.0f));
	vec4 lightVector = normalize(lightPosition - worldPosition);
	vec4 viewVector = normalize(viewPosition - worldPosition);
	// reflect(I, N) = I - 2.0 * dot(N, I) * N.
	vec4 reflectVector = reflect(-lightVector, worldNormal);
	float D = distance(worldPosition, lightPosition);

	/* Phong reflection model */
	vec4 diffuse = max(dot(worldNormal, lightVector), 0) * light[DIFFUSE] * k[DIFFUSE];
	vec4 specular = pow(max(dot(reflectVector, viewVector), 0), shininess) * light[SPECULAR] * k[SPECULAR];
	float distanceFactor = d_factor[0] + d_factor[1] * D + d_factor[2] * D * D;
	phongColor = (specular + diffuse) / distanceFactor + light[AMBIENT] * k[AMBIENT];

	gl_Position = vp * worldPosition;
}
