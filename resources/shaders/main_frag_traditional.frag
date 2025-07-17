#version 450 
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 2, binding = 0) uniform sampler2D albedo;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location=0) out vec4 outColor;

void main() {
	outColor = texture(albedo, fragTexCoord);
}