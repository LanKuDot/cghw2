#version 330

// Default color buffer location is 0
// If you create framebuffer your own, you need to take care of it
layout(location=0) out vec4 color;

in vec2 fTexcoord;
in vec4 worldPosition;
in vec4 worldNormal;

uniform sampler2D uSampler;

void main()
{
	color = texture(uSampler,fTexcoord);
}
