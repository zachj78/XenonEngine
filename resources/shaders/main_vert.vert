#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos; 
    vec3 lightColor; 
    vec3 cameraPos; 
} ubo;

layout(set = 1, binding = 0) readonly buffer MeshStorage {
    mat4 modelMatrices[];
};

layout(push_constant) uniform PushConstants {
    int meshIndex;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    mat4 model = modelMatrices[pc.meshIndex];
    gl_Position = model * ubo.proj * ubo.view * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}