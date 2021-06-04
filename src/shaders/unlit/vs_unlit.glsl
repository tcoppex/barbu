#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inAttrib;

layout(location = 0) out vec3 outColor;

// Uniforms.
uniform mat4 uMVP;

// ----------------------------------------------------------------------------

void main() {
  gl_Position = uMVP * vec4(inPosition, 1.0);
  outColor = 0.5 * inAttrib + 0.5;
}

// ----------------------------------------------------------------------------