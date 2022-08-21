#version 430 core
precision highp float;

// ----------------------------------------------------------------------------
//
// Performs a square blur on a RGBA8 texture, filtered by alpha value.
// * Note that an iterative square blur can be used to approximate a Gaussian blur.
//
// ----------------------------------------------------------------------------

// Speed-up the shader by using a fixed radius at compile-time.
#define USE_STATIC_PERFORMANCE

// ----------------------------------------------------------------------------
// -- STATIC PARAMETERS (do not change settings manually here)

#ifndef BLOCK_DIM
# define BLOCK_DIM      32
#endif

#ifndef BLUR_MAX_RADIUS
# define BLUR_MAX_RADIUS     BLOCK_DIM
#endif

#define TILE_MAXDIM     (BLOCK_DIM + 2 * BLUR_MAX_RADIUS)
#define SMEM_SIZE       (TILE_MAXDIM * TILE_MAXDIM)

layout(local_size_x = BLOCK_DIM, local_size_y = BLOCK_DIM) in;

// ----------------------------------------------------------------------------
// -- PARAMETERS

// Using a sampler as source instead of an image allows for texture clamping.
uniform sampler2D uSrcTex;

writeonly 
uniform layout(rgba8) image2D uDstImg;

#ifdef USE_STATIC_PERFORMANCE
#define uRadius  20
#else
uniform int uRadius;
#endif

// ----------------------------------------------------------------------------
// -- CONSTANTS

const vec2 kTexelSize = 1.0f / textureSize(uSrcTex, 0).xy;

// ----------------------------------------------------------------------------
// -- SHARED MEMORY
  
// Tile used for the blur kernel computation.
// Pixels value (RGBA) are stored as uint (4*byte) to save memory.
shared uint s_tile[SMEM_SIZE];

// Store weither or not the pixels corners has alpha. Used to avoid processing invisible pixels.
shared uint s_hasAlpha[BLOCK_DIM * BLOCK_DIM];

// ----------------------------------------------------------------------------
// -- MACROS

// Easy access to shared memory.
#define SMEM(X, Y)       s_tile[(Y) * TILE_MAXDIM + (X)]

// Load texel from unormalize texture coordinates.
#define LOADTEXEL(X, Y)  texture2D(uSrcTex, ivec2(X,Y) * kTexelSize, 0)

// Convert back a pixel from shared memory uint to vec3.
#define GETPIXEL(X, Y)   unpackUnorm4x8(SMEM(X, Y))

// Pack a pixel from src into an uint to store in shared memory.
// #define SETPIXEL(X, Y)   packUnorm4x8(LOADTEXEL(X, Y))

uint SETPIXEL(int x, int y) {
  const vec4 rgba = LOADTEXEL(x, y);
  s_hasAlpha[gl_LocalInvocationIndex] |= uint(rgba.a > 0.0); 
  return packUnorm4x8(rgba);
}

// ----------------------------------------------------------------------------
// -- MAIN

void main() {
  const ivec3 gridDim   = ivec3(gl_NumWorkGroups);
  const ivec3 blockDim  = ivec3(gl_WorkGroupSize);
  const ivec3 blockIdx  = ivec3(gl_WorkGroupID);
  const ivec3 threadPos = ivec3(gl_GlobalInvocationID);
  const ivec3 threadIdx = ivec3(gl_LocalInvocationID);

  const int tx = threadIdx.x;
  const int ty = threadIdx.y;
  const int bw = blockDim.x;
  const int bh = blockDim.y;
  const int x = threadPos.x;
  const int y = threadPos.y;
  const int r = uRadius;

  // ---------------------------------
  // Initialize the shared memory :
  //  Each pixel-thread will store pixels at radius distance on its 8 cardinal directions.

  s_hasAlpha[gl_LocalInvocationIndex] = 0;

  SMEM(tx + r, ty + r) = SETPIXEL(x, y);

  if (threadIdx.x < r) {
    // left
    SMEM(tx, ty + r) = SETPIXEL(x - r, y);
    // right
    SMEM(r + bw + tx, ty + r) = SETPIXEL(x + bw, y);
  }

  if (threadIdx.y < r) {
    // top
    SMEM(tx + r, ty) = SETPIXEL(x, y - r);
    // bottom
    SMEM(tx + r, r + bh + ty) = SETPIXEL(x, y + bh);
  }

  if ((threadIdx.x < r) && (threadIdx.y < r)) {
    // top-left
    SMEM(tx, ty) = SETPIXEL(x - r, y - r);
    // bottom-left
    SMEM(tx, r + bh + ty) = SETPIXEL(x - r, y + bh);
    // top-right
    SMEM(r + bw + tx, ty) = SETPIXEL(x + bh, y - r);
    // bottom-right
    SMEM(r + bw + tx, r + bh + ty) = SETPIXEL(x + bw, y + bh);
  }

  barrier();

  // ---------------------------------
  // Filter out the group without any blur to do.
  // [Alternative : use two passes with tile binning.]

  if (0 == gl_LocalInvocationIndex) {
    uint has_alpha = s_hasAlpha[0];
    for (int i = 1; i < blockDim.x * blockDim.y; ++i) {
      has_alpha |= s_hasAlpha[i];
    }
    s_hasAlpha[0] = has_alpha;
  }
  barrier();

  const bool do_blur = s_hasAlpha[0] > 0;

  // ---------------------------------

  const ivec2 pt = ivec2(tx + r, ty + r);
  
  // [todo] Based it on depth.
  const int blur_step = 3;

  vec4 rgba = vec4(0.0);
  if (do_blur) {
    const int r2 = r * r; 
    float samples = 0;

    for (int dy = -r; dy <= r; dy += blur_step) {
      int dy_square = dy * dy;
      for (int dx = -r; dx <= r; dx += blur_step) {
        int dp = dy_square + dx * dx;
        if (dp <= r2) {
          float k = 1.0 - dp / float(r2); //
          rgba += k * GETPIXEL(pt.x + dx, pt.y + dy);
          samples += k;
        }
      }
    }
    rgba /= float(samples + step(samples, 0));
  }
  
  imageStore(uDstImg, threadPos.xy, rgba);
}

// ----------------------------------------------------------------------------
