#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos; 
    vec3 lightColor; 
    vec3 cameraPos; 
} ubo;

layout(std430, set = 1, binding = 0) readonly buffer MeshStorage {
    mat4 modelMatrices[];
};

layout(std430, push_constant) uniform PushConstants {
    int meshIndex;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent; 
layout(location = 4) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

void main() {
    mat4 model = modelMatrices[pc.meshIndex];

    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec3 normal = normalize(normalMatrix * inNormal);
    vec3 tangent = normalize(normalMatrix * inTangent.xyz);
    vec3 bitangent = cross(normal, tangent) * inTangent.w; 

    fragColor = inColor.rgb;
    fragTexCoord = inTexCoord;
    fragNormal = normal; 
    fragTangent = tangent; 
    fragBitangent = bitangent; 

    gl_Position = model * ubo.proj * ubo.view * vec4(inPosition, 1.0);
}