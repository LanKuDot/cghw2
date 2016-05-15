#version 330

layout(location=0) out vec4 color;

in vec2 fTexcoord;
in vec2 pos;

uniform sampler2D uSampler;
uniform float circleRadius;	// The radius of the circle centering at cursorPos.
uniform float magnifyFactor;
uniform vec2 cursorPos;
uniform vec2 viewSize;
uniform mat3 gaussianKernel;

float distanceModified(vec2 p0, vec2 p1)
{
	return sqrt(pow(p0.x - p1.x, 2) + pow((p0.y - p1.y) * viewSize.y/viewSize.x, 2));
}

vec4 gaussianBlur()
{
	vec2 sampleTexcoord;
	vec4 blurColor = vec4(0.0f);
	vec2 diff = pos - cursorPos;
	diff.y = diff.y * viewSize.y/viewSize.x;

	for (int x = 0; x < 3; ++x)
		for (int y = 0; y < 3; ++y) {
			sampleTexcoord =  fTexcoord - diff*magnifyFactor;
			// Get neighbor pixel
			// Because the fTexcoord is normalized into [0,1],
			// 1 pixel up/down is 1/Height and 1 pixel left/right is 1/Width.
			sampleTexcoord.x += (x-1) / viewSize.x;
			sampleTexcoord.y += (y-1) / viewSize.y;
			blurColor += texture(uSampler, sampleTexcoord) * gaussianKernel[x][y];
		}

	return blurColor;
}

void main()
{
	if (distanceModified(pos, cursorPos) < circleRadius) {
		// Make the pixel in the circle pick the inner texture.
		color = gaussianBlur();
	} else
		color = texture(uSampler, fTexcoord);
}
