#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inTexcoord;

// Outputs.
layout(location = 0) out vec4 fragColor;

// Uniforms.
uniform samplerCube uCubemap;

// ----------------------------------------------------------------------------

void main() {
  fragColor = texture(uCubemap, inTexcoord, 2);
}

// ----------------------------------------------------------------------------
