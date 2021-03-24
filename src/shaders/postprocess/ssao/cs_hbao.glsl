#version 430 core

#include "shared/inc_maths.glsl"  
#include "postprocess/ssao/interop.h"

//------------------------------------------------------------------------------
//  UNIFORMS 

uniform vec4 uAOResolution;
uniform vec4 uUVToView;
uniform float uR2;
uniform float uTanAngleBias;
uniform float uStrength; // only HBAO Y

uniform sampler2D uTexLinearDepth;

writeonly uniform layout(r32f)  image2D uImgOutputX;      // output pass 1

readonly  uniform layout(r32f)  image2D uImgInputX;       // input pass 2
writeonly uniform layout(rg16f) image2D uImgOutputXY;     // output pass 2

subroutine void HBAOFunction(ivec3 threadIdx, ivec3 blockIdx);
subroutine uniform HBAOFunction suHBAO;

//------------------------------------------------------------------------------
// STATIC PARAMETERS

// Step size in number of pixels.
#ifndef STEP_SIZE
# define STEP_SIZE            4
#endif

// Number of shared-memory samples per direction.
#ifndef NUM_STEPS
# define NUM_STEPS            8
#endif

// Maximum kernel radius in number of pixels.
#ifndef KERNEL_RADIUS
# define KERNEL_RADIUS        (NUM_STEPS*NUM_STEPS)
#endif

#define SMEM_SIZE             (HBAO_TILE_WIDTH + 2 * KERNEL_RADIUS)

// The last sample has weight = exp(-KERNEL_FALLOFF)
#define KERNEL_FALLOFF        3.0f

//------------------------------------------------------------------------------
//  SHARED MEMORY

shared vec2 smem[SMEM_SIZE];

//------------------------------------------------------------------------------
//  MACRO

#define SMEM(x)             smem[x]

//------------------------------------------------------------------------------

float tangent(in vec2 v) {
  return -v.y / (abs(v.x) + Epsilon());
}

float tanToSin(float x) {
  return x * inversesqrt(x*x + 1.0f);
}

float falloff(float sampleId) {
  const float r = sampleId / (NUM_STEPS - 1);
  return exp(-KERNEL_FALLOFF*r*r);
}

vec2 minDiff(in vec2 p, in vec2 pr, in vec2 pl) {
  const vec2 v1 = pr - p;
  const vec2 v2 = p - pl;
  return (dot(v1, v1) < dot(v2, v2)) ? v1 : v2;
}

float integrateDirection(float ao, vec2 p, float tanT, int threadId, ivec2 stepSize) {
  float tanH = tanT;
  float sinH = tanToSin(tanH);

  // Per-sample attenuation.
  #pragma unroll
  for (int sampleId = 0; sampleId < NUM_STEPS; ++sampleId)
  {
    const vec2 s = SMEM(threadId + sampleId*stepSize.y + stepSize.x);
    const vec2 v = s - p;
    const float tanS = tangent(v);

    if ((dot(v, v) < uR2) && (tanS > tanH))
    {
      // Accumulate AO between the horizon and the sample.
      const float sinS = tanToSin(tanS);
      ao += falloff(sampleId) * (sinS - sinH);

      // Update the current horizon angle.
      tanH = tanS;
      sinH = sinS;
    }
  }

  return ao;
}

//------------------------------------------------------------------------------
// Bias tangent angle and compute HBAO in the +/- X or  +/- Y directions.
//------------------------------------------------------------------------------
float computeHBAO(vec2 p, vec2 t, int centerId) {
  const float tanT = tangent(t);
  const ivec2 stepSize = ivec2(STEP_SIZE);

  float ao = 0.0f;
  ao += integrateDirection(ao, p, uTanAngleBias + tanT, centerId, +stepSize);
  ao += integrateDirection(ao, p, uTanAngleBias - tanT, centerId, -stepSize);
  
  return ao;
}

//------------------------------------------------------------------------------
// Compute view-space coordinates from the depth texture
//------------------------------------------------------------------------------
vec2 fetchVSfromDepth(in ivec2 xy, in vec2 sel) {
  
  const vec2 uv = (vec2(xy) + 0.5f) * uAOResolution.zw;
  const float z_eye = texture(uTexLinearDepth, uv).r;

  const vec2 xy_eye = (uUVToView.xy * uv + uUVToView.zw) * z_eye;
  return vec2( dot(xy_eye, sel), z_eye);
}

vec2 fetchXZ(int x, int y) {
  return fetchVSfromDepth( ivec2(x, y), vec2(1, 0));
}

vec2 fetchYZ(int x, int y) {
  return fetchVSfromDepth( ivec2(x, y), vec2(0, 1));
}

//------------------------------------------------------------------------------
// Compute HBAO for the left and right directions
//------------------------------------------------------------------------------
subroutine(HBAOFunction)
void HBAO_X(ivec3 threadIdx, ivec3 blockIdx) {
  const int tileStart  = blockIdx.x * HBAO_TILE_WIDTH;
  const int tileEnd    = tileStart + HBAO_TILE_WIDTH;
  const int apronStart = tileStart - KERNEL_RADIUS;
  const int apronEnd   = tileStart + KERNEL_RADIUS;

  const int x = apronStart + threadIdx.x;
  const int y = blockIdx.y;

  // Initialize shared memory tile.
  const int next_id = min( threadIdx.x + 2*KERNEL_RADIUS, SMEM_SIZE-1); 
  SMEM(threadIdx.x) = fetchXZ(x, y);
  SMEM(next_id)     = fetchXZ(x + 2*KERNEL_RADIUS, y);

  /*-------*/ barrier(); /*------------*/

  const ivec2 threadPos = ivec2(tileStart + threadIdx.x, blockIdx.y);  
  const int tileEndClamped = min(tileEnd, int(uAOResolution.x));

  if (threadPos.x < tileEndClamped)
  {
    const int centerId = threadIdx.x + KERNEL_RADIUS;

    const vec2 p  = SMEM(centerId);
    const vec2 pr = SMEM(centerId+1);
    const vec2 pl = SMEM(centerId-1);

    // Compute tangent vector using central differences.
    const vec2 t = minDiff(p, pr, pl);
    
    float ao = computeHBAO(p, t, centerId);
    imageStore(uImgOutputX, threadPos, ao.xxxx);
  }
}

//------------------------------------------------------------------------------
// Compute HBAO for the up and down directions.
// Output the average AO for the 4 axis-aligned directions.
//------------------------------------------------------------------------------
subroutine(HBAOFunction)
void HBAO_Y(ivec3 threadIdx, ivec3 blockIdx) {
  const int tileStart  = blockIdx.x * HBAO_TILE_WIDTH;
  const int tileEnd    = tileStart + HBAO_TILE_WIDTH;
  const int apronStart = tileStart - KERNEL_RADIUS;
  const int apronEnd   = tileStart + KERNEL_RADIUS;

  const int x = blockIdx.y;
  const int y = apronStart + threadIdx.x;

  // Initialize shared memory tile.
  const int next_id = min( threadIdx.x + 2*KERNEL_RADIUS, SMEM_SIZE-1); 
  SMEM(threadIdx.x) = fetchYZ(x, y);
  SMEM(next_id)     = fetchYZ(x, y + 2*KERNEL_RADIUS);

  /*-------*/ barrier(); /*------------*/

  const ivec2 threadPos = ivec2(blockIdx.y, tileStart + threadIdx.x);
  const int tileEndClamped = min(tileEnd, int(uAOResolution.y));

  if (threadPos.y < tileEndClamped)
  {
    const int centerId = threadIdx.x + KERNEL_RADIUS;

    const vec2 p  = SMEM(centerId);
    const vec2 pt = SMEM(centerId + 1);
    const vec2 pb = SMEM(centerId - 1);

    // Compute tangent vector using central differences.
    const vec2 t = minDiff(p, pt, pb);

    const float aoy = computeHBAO(p, t, centerId);
    const float aox = imageLoad(uImgInputX, threadPos ).r;
    
    float ao = 1.0f - 0.25f * uStrength * (aox + aoy);
    ao = clamp( ao, 0.0f, 1.0f);

    imageStore(uImgOutputXY, threadPos, vec4(ao, p.y, 0.0f, 0.0f));
  }
}

//------------------------------------------------------------------------------

layout(local_size_x = HBAO_TILE_WIDTH) in;
void main() {
  const ivec3 threadIdx = ivec3(gl_LocalInvocationID);
  const ivec3 blockIdx  = ivec3(gl_WorkGroupID);
  suHBAO(threadIdx, blockIdx);
}

//------------------------------------------------------------------------------
