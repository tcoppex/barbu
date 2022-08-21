#version 430 core

#include "postprocess/ssao/inc_blur_ao_shared.glsl"

// ----------------------------------------------------------------------------

writeonly uniform layout(r32f) image2D uDstImg;

layout(local_size_x = HBAO_BLUR_BLOCK_DIM) in;

void main() {
  float writePos_out;
  vec4 ao_y_out;

  if (blurPass(1.0f, writePos_out, ao_y_out)) {
    ivec2 coords = ivec2(int(gl_WorkGroupID.y), writePos_out);
    imageStore(uDstImg, coords, ao_y_out);
  }
}

// ----------------------------------------------------------------------------