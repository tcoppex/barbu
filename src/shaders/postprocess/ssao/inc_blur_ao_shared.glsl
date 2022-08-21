#ifndef SHADERS_POSTPROCESS_SSAO_INC_BLUR_AO_SHARED_GLSL_
#define SHADERS_POSTPROCESS_SSAO_INC_BLUR_AO_SHARED_GLSL_

#include "postprocess/ssao/interop.h"

// ----------------------------------------------------------------------------
//  UNIFORMS 

uniform float uBlurFalloff;
uniform float uBlurDepthThreshold;
uniform vec4 uResolution;

uniform sampler2D uTexAONearest;
uniform sampler2D uTexAOLinear;

// ----------------------------------------------------------------------------
//  STATIC PARAMETERS

#ifndef KERNEL_RADIUS
# define KERNEL_RADIUS      HBAO_BLUR_RADIUS
#endif

#define SMEM_SIZE           HBAO_BLUR_BLOCK_DIM

// ----------------------------------------------------------------------------
//  CONSTANTS

const int kBlurRadius     = KERNEL_RADIUS;
const int kHalfBlurRadius = kBlurRadius / 2;

// ----------------------------------------------------------------------------
//  SHARED MEMORY

shared vec2 smem[SMEM_SIZE];

// ----------------------------------------------------------------------------
//  MACROS

#define TEX_AOZ_NEAREST(uv)   texture(uTexAONearest, uv, 0).rg
#define TEX_AOZ_LINEAR(uv)    texture(uTexAOLinear,  uv, 0).rg
#define SMEM(x)               smem[x]

// ----------------------------------------------------------------------------

float CrossBilateralWeight(float r, float d, float d0) {
  return exp2(-r*r*uBlurFalloff) * (1.0f - step(uBlurDepthThreshold, abs(d - d0)));
}

// ----------------------------------------------------------------------------

bool blurPass(float switchXY, out float writePos_, out vec4 ao_) {
  const ivec3 threadIdx = ivec3(gl_LocalInvocationID);
  const ivec3 blockIdx  = ivec3(gl_WorkGroupID);

  const float        row = float(blockIdx.y);
  const float  tileStart = float(blockIdx.x) * HBAO_TILE_WIDTH;
  const float    tileEnd = tileStart + HBAO_TILE_WIDTH;
  const float apronStart = tileStart - KERNEL_RADIUS;
  const float   apronEnd =   tileEnd + KERNEL_RADIUS;

  // ---------------------------------

  const float x = apronStart + float(threadIdx.x) + 0.5f;
  const float y = row;
  vec2 uv_base = vec2(x, y);
       uv_base = mix(uv_base, uv_base.yx, switchXY);
  vec2 uv = (uv_base + 0.5f) * uResolution.zw;
  
  SMEM(threadIdx.x) = TEX_AOZ_LINEAR(uv);
  barrier();

  // ---------------------------------

  writePos_ = tileStart + float(threadIdx.x);
  const float tileEndClamped = min(tileEnd, uResolution.x);

  if (writePos_ < tileEndClamped) 
  {
    // Fetch (ao, z) at the kernel center.
    uv = mix(vec2(writePos_, uv_base.y), vec2(uv_base.x, writePos_), switchXY);
    uv = (uv + 0.5f) * uResolution.zw;
    const vec2 aoDepth = TEX_AOZ_NEAREST(uv);
    const float center_d = aoDepth.y;

    float ao_total = aoDepth.x;
    float w_total = 1.0f;

    #pragma unroll
    for (int i = 0; i < kHalfBlurRadius; ++i)
    {
      // Sample the pre-filtered data with step size = 2 pixels
      const float r = 2.0f * i + (0.5f - kBlurRadius);
      const uint j = 2u * i + threadIdx.x;
      const vec2 samp = SMEM(j);
      const float w = CrossBilateralWeight(r, samp.y, center_d);
      ao_total += w * samp.x;
      w_total  += w;
    }

    #pragma unroll
    for (int i = 0; i < kHalfBlurRadius; ++i)
    {
      // Sample the pre-filtered data with step size = 2 pixels
      const float r = 2.0f * i + 1.5f;
      const uint j = 2u * i + threadIdx.x + KERNEL_RADIUS + 1u;
      const vec2 samp = SMEM(j);
      const float w = CrossBilateralWeight(r, samp.y, center_d);
      ao_total += w * samp.x;
      w_total  += w;
    }

    ao_ = vec4(ao_total / w_total, center_d, 0.0f, 1.0f);

    return true;
  }

  return false;
}

// ----------------------------------------------------------------------------

#endif // SHADERS_POSTPROCESS_SSAO_INC_BLUR_AO_SHARED_GLSL_
