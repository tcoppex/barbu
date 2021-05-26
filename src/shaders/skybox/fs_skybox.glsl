#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inView;

// Outputs.
layout(location = 0) out vec4 fragColor;

// Uniforms.
uniform samplerCube uCubemap;
uniform vec4 uFactors = vec4(1.0);

// ----------------------------------------------------------------------------

void main() {
  fragColor = uFactors * textureLod(uCubemap, inView, 0.0);
}

// ----------------------------------------------------------------------------
