#version 430 core

#include "postprocess/ssao/inc_blur_ao_shared.glsl"

// ----------------------------------------------------------------------------

writeonly uniform layout(r32f) image2D uDstImg;

layout(local_size_x = HBAO_BLUR_BLOCK_DIM) in;
#if 0
// [wip on generic version, not there yet]
void main() {
  float writePos_out;
  vec4 ao_y_out;

  if (blurPass(1.0f, writePos_out, ao_y_out)) {
    imageStore( uDstImg, ivec2(int(gl_WorkGroupID.y), writePos_out), ao_y_out);
  }
}
#else
void main()
{
  const ivec3 threadIdx  = ivec3(gl_LocalInvocationID);
  const ivec3 blockIdx   = ivec3(gl_WorkGroupID);

  const float        col = float(blockIdx.y);
  const float  tileStart = float(blockIdx.x) * HBAO_TILE_WIDTH;
  const float    tileEnd = tileStart + HBAO_TILE_WIDTH;
  const float apronStart = tileStart - KERNEL_RADIUS;
  const float   apronEnd = tileEnd + KERNEL_RADIUS;


  const float x = col;
  const float y = apronStart + float(threadIdx.x) + 0.5f;
  const vec2 uv = (vec2(x, y) + 0.5f) * uResolution.zw; //
  SMEM(threadIdx.x) = TEX_AOZ_LINEAR(uv);

  /*-------*/ barrier(); /*-------*/

  const float writePos = tileStart + float(threadIdx.x);
  const float tileEndClamped = min(tileEnd, uResolution.x);

  if (writePos < tileEndClamped)
  {
    vec2 uv = (vec2(x, writePos) + 0.5f) * uResolution.zw;//
    vec2 aoDepth = TEX_AOZ_NEAREST(uv);
    float ao_total = aoDepth.x;
    float center_d = aoDepth.y;
    float w_total = 1.0f;

#   pragma unroll
    for (int i = 0; i < kHalfBlurRadius; ++i)
    {
      float r = 2.0f*i + (-kBlurRadius + 0.5f);
      uint j = 2u*i + threadIdx.x;
      vec2 samp = SMEM(j);
      float w = CrossBilateralWeight(r, samp.y, center_d);
      ao_total += w * samp.x;
      w_total += w;
    }

#   pragma unroll
    for (int i = 0; i < kHalfBlurRadius; ++i)
    {
      float r = 2.0f * i + 1.5f;
      uint j = 2u*i + threadIdx.x + KERNEL_RADIUS + 1u;
      vec2 samp = SMEM(j);
      float w = CrossBilateralWeight(r, samp.y, center_d);
      ao_total += w * samp.x;
      w_total += w;
    }

    float ao = ao_total / w_total;
    imageStore(uDstImg, ivec2(blockIdx.y, writePos), ao.xxxx);
  }
}
#endif
// ----------------------------------------------------------------------------