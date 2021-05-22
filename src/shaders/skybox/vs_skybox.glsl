#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition;

// Outputs.
layout(location = 0) out vec3 outView;

// Uniforms.
uniform mat4  uMVP;

// ----------------------------------------------------------------------------

void main() {
  const vec4 pos = vec4(inPosition, 1.0);
  const vec4 clip_pos = uMVP * pos;

  // Use the box object coordinates in [-1, 1] as texture coordinates.
  outView = normalize(inPosition);

  // Assure depth value is always at 1.0.
  gl_Position = clip_pos.xyww;
}

// ----------------------------------------------------------------------------
