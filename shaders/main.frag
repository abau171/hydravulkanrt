#version 460

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in flat vec3 fragColor;
layout(location = 2) in vec3 fragPosition;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outWorldPosition;
layout(location = 2) out vec4 outWorldNormal;

void main() {
    outAlbedo = vec4(fragColor, 1.0f);
    outWorldPosition = vec4(fragPosition, 1.0f);
    outWorldNormal = vec4(normalize(fragNormal), 1.0f);
}
