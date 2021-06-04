#version 430 core

// ----------------------------------------------------------------------------

layout(location = 0) in vec3 inColor;
layout(location = 0) out vec4 fragColor;

// Uniforms.
uniform vec4 uColor = vec4(0.75, 0.5, 0.5, 1.0);
uniform bool uUseAttribColor = false;

// ----------------------------------------------------------------------------

void main() {
  fragColor = uUseAttribColor ? vec4(inColor, 1.0) : uColor;
}

// ----------------------------------------------------------------------------
