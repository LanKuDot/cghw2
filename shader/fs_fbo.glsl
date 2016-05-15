#version 330

layout(location=0) out vec4 color;

in vec2 fTexcoord;
in vec2 pos;

uniform sampler2D uSampler;
uniform float circleRadius;	// The radius of the circle centering at cursorPos.
uniform float viewRatioYtoX;
uniform float magnifyFactor;
uniform vec2 cursorPos;

float distanceModified(vec2 p0, vec2 p1)
{
	return sqrt(pow(p0.x - p1.x, 2) + pow((p0.y - p1.y) * viewRatioYtoX, 2));
}

void main()
{
	if (distanceModified(pos, cursorPos) < circleRadius) {
		vec2 diff = pos - cursorPos;
		diff.y = diff.y * viewRatioYtoX;
		// Make the pixel in the circle pick the inner texture.
		color = texture(uSampler, fTexcoord - diff*magnifyFactor);
	} else
		color = texture(uSampler, fTexcoord);
}
