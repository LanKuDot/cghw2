#version 330

#define AMBIENT 0
#define DIFFUSE 1
#define SPECULAR 2

layout(location=0) out vec4 color;

in vec2 fTexcoord;
in vec4 worldNormal;
in vec4 worldPosition;

uniform vec4 viewPosition;
uniform vec4 lightPosition;
uniform vec4 light[3];
uniform vec4 k[3];	// The refelection factor of material
uniform float shininess;
uniform float d_factor[3];	// The factor of distance quadratic funtion.

uniform sampler2D uSampler;

void main()
{
	/* The information which phong reflection model needed. */
	vec4 lightVector = normalize(lightPosition - worldPosition);
	vec4 viewVector = normalize(viewPosition - worldPosition);
	// reflect(I, N) = I - 2.0 * dot(N, I) * N.
	vec4 reflectVector = reflect(-lightVector, worldNormal);
	float D = distance(worldPosition, lightPosition);

	/* Phong reflection model */
	vec4 diffuse = max(dot(worldNormal, lightVector), 0) * light[DIFFUSE] * k[DIFFUSE];
	vec4 specular = pow(max(dot(reflectVector, viewVector), 0), shininess) * light[SPECULAR] * k[SPECULAR];
	float distanceFactor = d_factor[0] + d_factor[1] * D + d_factor[2] * D * D;
	vec4 phongColor = (specular + diffuse) / distanceFactor + light[AMBIENT] * k[AMBIENT];

	color = phongColor * texture(uSampler, fTexcoord);
}
