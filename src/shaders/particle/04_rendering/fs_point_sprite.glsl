#version 430 core

// ----------------------------------------------------------------------------

#include "particle/04_rendering/inc_shared.glsl"

// ----------------------------------------------------------------------------

in VDataBlock {
  vec3 position; // unused legacy of vs_generic
  vec3 velocity; // idem
  vec3 color;
  float decay;
  float pointSize;
} IN;

layout(location = 0) out vec4 fragColor;

// ----------------------------------------------------------------------------

void main() {
  fragColor = compute_color(IN.color, IN.decay, gl_PointCoord);
}

// ----------------------------------------------------------------------------
