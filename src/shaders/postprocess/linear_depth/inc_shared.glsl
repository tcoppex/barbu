#ifndef SHADERS_POSTPROCESS_LINEAR_DEPTH_INC_SHARED_H_
#define SHADERS_POSTPROCESS_LINEAR_DEPTH_INC_SHARED_H_

// ----------------------------------------------------------------------------

// [Ideally set externally]
#ifndef USE_OPENGL
# define USE_OPENGL  1
#endif

// X : z_near
// Y : z_far
// Z : A = z_far / (z_far - z_near)
// W : B = - z_near * A
uniform vec4 uLinearParams;

// ----------------------------------------------------------------------------
// for NDC in [-1, 1] (OpenGL).

float LinearizeDepth_GL(float z, float n, float f) {
  return (2 * n) / (n + f + z * (n - f));
}

// ----------------------------------------------------------------------------
// for NDC in [0, 1] (Vulkan / DirectX).

float LinearizeDepth_VK(float z, float n, float f) {
  return (n * f) / (f + z * (n - f));
}

float LinearizeDepthFast_VK(float z, float A, float B) {
  return B / (z - A);
}

// ----------------------------------------------------------------------------

float LinearizeDepth(float z, float n, float f) {
#if USE_OPENGL
  return LinearizeDepth_GL(z, n, f);
#else
  return LinearizeDepth_VK(z, n, f);
#endif
}

float LinearizeDepth(float z) {
#if USE_OPENGL
  return LinearizeDepth_GL(z, uLinearParams.x, uLinearParams.y);
#else
  return LinearizeDepthFast_VK(z, uLinearParams.z, uLinearParams.w);
#endif
}

// ----------------------------------------------------------------------------

#endif // SHADERS_POSTPROCESS_LINEAR_DEPTH_INC_SHARED_H_