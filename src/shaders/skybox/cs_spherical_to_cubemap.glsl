#version 430 core
precision highp float;

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------
//
// Create a cubemap texture from a spherical image.
//
// The shader expects a grid of the resolution of the cubemap in XY, and the
// number of faces (6) on Z.
//
// ref : http://paulbourke.net/panorama/cubemaps/#3
//
// ----------------------------------------------------------------------------

uniform sampler2D uSphericalTex;
uniform int uResolution;
uniform mat4 uFaceViews[6];

writeonly 
uniform layout(rgba16f) imageCube uDstImg;

// ----------------------------------------------------------------------------

vec2 cubemapToSphericalCoords(in vec3 v) {
  const vec2 kInvAtan = vec2(0.1591, 0.3183);
  const vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  return fma(uv, kInvAtan, vec2(0.5));
}

// ----------------------------------------------------------------------------

#ifndef BLOCK_DIM
  #define BLOCK_DIM      16
#endif

layout(local_size_x = BLOCK_DIM, local_size_y = BLOCK_DIM, local_size_z = 1) in;
void main()
{
  const ivec3 coords = ivec3(gl_GlobalInvocationID.xyz);

  // Check boundaries.
  if (!all(lessThan(coords, uvec3(uResolution, uResolution, 6)))) {
    return;
  }

  // Textures coordinates in [-1, 1].
  vec2 uv = 2.0 * (coords.xy * vec2(1.0 / uResolution)) - vec2(1.0);

  // Flip y-axis.
  uv.y = - uv.y; //

  // Find the view ray from camera.
  vec3 view = normalize(vec3(uv, -1.0)); //
       view = normalize(vec3(uFaceViews[coords.z] * vec4(view, 0.0)));

  // Spherical (equirectangular) map sample.
  const vec2 spherical_coords = cubemapToSphericalCoords(view);
  const vec3 data = texture( uSphericalTex, spherical_coords).rgb;

  imageStore( uDstImg, coords, vec4( data, 1.0));
}

// ----------------------------------------------------------------------------