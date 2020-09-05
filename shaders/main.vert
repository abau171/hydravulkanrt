#version 460

layout(push_constant) uniform PushConstants {
    mat4 modelToNdc;
    mat4 modelToWorld;
    mat3 normalModelToWorld;
    vec3 color;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out flat vec3 fragColor;
layout(location = 2) out vec3 fragPosition;

void main() {
    gl_Position = pushConstants.modelToNdc * vec4(inPosition, 1.0f);
    fragNormal = normalize(pushConstants.normalModelToWorld * inNormal);
    fragColor = pushConstants.color;
    vec4 fragPositionHomog = pushConstants.modelToWorld * vec4(inPosition, 1.0f);
    fragPosition = fragPositionHomog.xyz / fragPositionHomog.w;
}
