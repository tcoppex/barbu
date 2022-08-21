#version 430 core

#include "postprocess/ssao/inc_blur_ao_shared.glsl"

// ----------------------------------------------------------------------------

writeonly uniform layout(rg16f) image2D uDstImg;

layout(local_size_x = HBAO_BLUR_BLOCK_DIM) in;

void main() {
  float writePos_out;
  vec4 ao_x_out;

  if (blurPass(0.0f, writePos_out, ao_x_out)) {
    ivec2 coords = ivec2(writePos_out, int(gl_WorkGroupID.y));
    imageStore(uDstImg, coords, ao_x_out);
  }
}

// ----------------------------------------------------------------------------