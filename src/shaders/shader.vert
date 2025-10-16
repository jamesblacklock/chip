#version 450

layout(binding = 0) uniform UBO {
  mat4 view;
  mat4 proj;
} ubo;

layout(push_constant) uniform PConst {
  mat4 model;
} pconst;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
  gl_Position = ubo.proj * ubo.view * pconst.model * vec4(inPosition, 0.0, 1.0);
  fragColor = inColor;
}
