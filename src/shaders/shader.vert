#version 450

layout(set = 0, binding = 0) uniform UBO {
  mat4 view;
  mat4 proj;
  vec2 dims;
} ubo;

struct Attr {
  mat4 transform;
  vec3 color;
};

layout(set = 0, binding = 1) readonly buffer TransformBuffer {
  Attr attrs[];
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in uint attrId;

layout(location = 0) out vec3 fragColor;

void main() {
  vec4 pos = ubo.proj * ubo.view * attrs[attrId].transform * vec4(inPosition, 0.0, 1.0);
  gl_Position = vec4(pos[0]/(ubo.dims[0]/2)/(pos[2]/1000), pos[1]/(ubo.dims[1]/2)/(pos[2]/1000), pos[2] < 0 ? -1 : 0, 1.0);
  fragColor = attrs[attrId].color;
}
