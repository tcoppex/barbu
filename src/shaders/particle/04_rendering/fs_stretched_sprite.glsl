#version 430 core

// ----------------------------------------------------------------------------

#include "particle/04_rendering/inc_shared.glsl"

// ----------------------------------------------------------------------------

in GDataBlock {
  vec3 color;
  vec2 texcoord;
  float decay;
} IN;

layout(location = 0) out vec4 fragColor;

// ----------------------------------------------------------------------------

void main() {
  fragColor = compute_color(IN.color, IN.decay, IN.texcoord);
}

// ----------------------------------------------------------------------------
