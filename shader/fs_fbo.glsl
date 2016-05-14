#version 330

layout(location=0) out vec4 color;

in vec2 fTexcoord;

uniform sampler2D uSampler;

void main()
{
	color = texture(uSampler,fTexcoord + 0.005 * vec2( sin(800*fTexcoord.x), cos(600*fTexcoord.y)));
}
