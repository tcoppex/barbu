#version 430 core

#include "postprocess/linear_depth/interop.h"
#include "postprocess/linear_depth/inc_shared.glsl"

// ----------------------------------------------------------------------------

uniform vec2 uResolution;

// [we can only sample hw depth via a sampler, a readonly image will not work].
uniform layout(binding = 0) sampler2D uDepthIn;

writeonly uniform layout(r32f) image2D uLinearDepthOut;

// ----------------------------------------------------------------------------

// Linearize depth from a single sample buffer (no MSAA).
layout(
  local_size_x = LINEARDEPTH_BLOCK_DIM, 
  local_size_y = LINEARDEPTH_BLOCK_DIM
) in;
void main() {
  const ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

  if (!all(lessThan(coords, uResolution))) {
    return;
  }

  const float depth = texture( uDepthIn, coords / uResolution).r;
  const float linear_depth = LinearizeDepth(depth);

  imageStore( uLinearDepthOut, coords, vec4(linear_depth));
}

// ----------------------------------------------------------------------------