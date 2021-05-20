#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inTexcoord;

// Outputs.
layout(location = 0) out vec4 fragColor;

// Uniforms.
uniform samplerCube uCubemap;
uniform vec4 uFactors = vec4(1.0);

// ----------------------------------------------------------------------------

void main() {
  fragColor = uFactors*texture(uCubemap, inTexcoord, 0);
}

// ----------------------------------------------------------------------------
