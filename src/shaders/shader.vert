#version 450

layout(set = 0, binding = 0) uniform UBO {
  mat4 view;
  mat4 proj;
} ubo;

struct TriangleAttr {
  mat4 transform;
  vec3 color;
};

layout(set = 0, binding = 1) readonly buffer TransformBuffer {
  TriangleAttr attrs[];
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in uint triangleId;

layout(location = 0) out vec3 fragColor;

void main() {
  gl_Position = ubo.proj * ubo.view * attrs[triangleId].transform * vec4(inPosition, 0.0, 1.0);
  fragColor = attrs[triangleId].color;
}
