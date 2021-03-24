#version 430 core

#include "postprocess/linear_depth/inc_shared.glsl"

// ----------------------------------------------------------------------------

in layout(location = 0) vec2 vTexCoord;

// [it seems mandatory to use a vec for the pass to work].
out layout(location = 0) vec4 fragDepth;

uniform sampler2D uDepthIn;

// ----------------------------------------------------------------------------

// Linearize depth from a single sample buffer (no MSAA).
void main() {
  const float depth = texture( uDepthIn, vTexCoord).r;
  const float linear_depth = LinearizeDepth(depth);
  fragDepth = vec4(linear_depth);
}

// ----------------------------------------------------------------------------