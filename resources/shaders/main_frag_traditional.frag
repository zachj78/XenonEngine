#version 450 

layout(set = 2, binding = 0) uniform sampler2D albedo;

layout(location=0) in vec3 fragColor;
layout(location=1) in vec2 fragTexCoord;

layout(location=0) out vec4 outColor;

void main() {
	outColor = texture(albedo, fragTexCoord);
}