#version 450 
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 2, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform PushConstants {
    int meshIndex;
} pc;

layout(location=0) in vec3 fragColor;
layout(location=1) in vec2 fragTexCoord;

layout(location=0) out vec4 outColor;

void main() {
	outColor = texture(textures[nonuniformEXT(pc.meshIndex)], fragTexCoord);
}