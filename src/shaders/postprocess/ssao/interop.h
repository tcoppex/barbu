#ifndef SHADERS_POSTPROCESS_SSAO_INTEROP_H_
#define SHADERS_POSTPROCESS_SSAO_INTEROP_H_

// ----------------------------------------------------------------------------

// (also HBAO pass BLOCK_DIM)
#ifndef HBAO_TILE_WIDTH
# define HBAO_TILE_WIDTH          320
#endif

#ifndef HBAO_BLUR_RADIUS
# define HBAO_BLUR_RADIUS         8
#endif

// ----------------------------------------------------------------------------

// [should probably be set & fetch at runtime]
#ifndef HBAO_BLUR_BLOCK_DIM
# define HBAO_BLUR_BLOCK_DIM      336 //(HBAO_TILE_WIDTH + 2 * HBAO_BLUR_RADIUS)
#endif

// ----------------------------------------------------------------------------

#endif // SHADERS_POSTPROCESS_SSAO_INTEROP_H_
