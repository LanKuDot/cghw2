#version 330

layout(location=0) out vec4 color;

in vec2 fTexcoord;
in vec4 phongColor;

uniform sampler2D uSampler;

void main()
{
	color = phongColor * texture(uSampler, fTexcoord);
}
