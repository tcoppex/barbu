#version 430 core
precision highp float;

#include "shared/inc_maths.glsl" 
#include "marching_cube/01_density_volume/inc_density.glsl"

// ----------------------------------------------------------------------------

uniform float uInvChunkDim;
uniform float uMargin;
uniform float uChunkSizeWS;
uniform vec3  uChunkPositionWS;

// ----------------------------------------------------------------------------

#define USE_SHADER_BLOCK 0

#if USE_SHADER_BLOCK

layout(std430, binding = 0)
writeonly buffer MarchingCube_DensityBuffer {
  float out_density[];
};

#else

writeonly 
uniform layout(r32f) image3D uDstImg;

#endif

// ----------------------------------------------------------------------------

#ifndef BLOCK_DIM
  #define BLOCK_DIM      4
#endif

layout(local_size_x = BLOCK_DIM, local_size_y = BLOCK_DIM, local_size_z = BLOCK_DIM) in;
void main()
{
  const ivec3 resolution = ivec3(gl_WorkGroupSize * gl_NumWorkGroups);
  const ivec3 coords     = ivec3(gl_GlobalInvocationID.xyz);
  const vec3 texcoords   = vec3(coords) / vec3(resolution);
  const int gid          = coords.z * resolution.x * resolution.y 
                         + coords.y * resolution.x 
                         + coords.x
                         ;

  // Check boundaries.
  if (!all(lessThan(coords, resolution))) {
    return;
  }

  // ---------------

  // Chunk coordinates with texels in the marge.
  const vec3 chunkcoord = (coords - uMargin) * uInvChunkDim;

  // World-space coordinates for the chunk.
  const vec3 chunkcoord_ws = uChunkSizeWS * chunkcoord + uChunkPositionWS;

  const float data = compute_density( chunkcoord_ws );

  // ---------------

#if USE_SHADER_BLOCK
  out_density[gid] = data;
#else
  imageStore( uDstImg, coords, vec4(data, 0.0f, 0.0f, 0.0f));
#endif
}

// ----------------------------------------------------------------------------