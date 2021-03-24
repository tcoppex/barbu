#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition;

// Outputs.
layout(location = 0) out vec3 outTexcoord;

// Uniforms.
uniform mat4  uMVP;

// ----------------------------------------------------------------------------

void main() {
  vec4 pos = vec4(inPosition, 1.0);
  gl_Position = uMVP * pos;

  // Use the box object coordinates in [-1, 1] as texture coordinates.
  outTexcoord = inPosition;
}

// ----------------------------------------------------------------------------
