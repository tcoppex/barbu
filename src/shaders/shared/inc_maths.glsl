#ifndef SHADERS_SHARED_INC_MATHS_GLSL_
#define SHADERS_SHARED_INC_MATHS_GLSL_

#include "shared/inc_constants.glsl"

// ----------------------------------------------------------------------------

float cbrt(float x) { 
  return pow(x, 0.33333);
}

// ----------------------------------------------------------------------------

mat3 rotationX(float c, float s) {
  return mat3(
    vec3(1, 0, 0),
    vec3(0, c, s),
    vec3(0, -s, c)
  );
}

mat3 rotationY(float c, float s) {
  return mat3(
    vec3(c, 0, -s),
    vec3(0, 1, 0),
    vec3(s, 0, c)
  );
}

mat3 rotationZ(float c, float s) {
  return mat3(
    vec3(c, s, 0),
    vec3(-s, c, 0),
    vec3(0, 0, 1)
  );
}

// ----------------------------------------------------------------------------

mat3 rotationX(float radians) {
  return rotationX(cos(radians), sin(radians));
}

mat3 rotationY(float radians) {
  return rotationY(cos(radians), sin(radians));
}

mat3 rotationZ(float radians) {
  return rotationZ(cos(radians), sin(radians));
}

// ----------------------------------------------------------------------------

mat3 basis_from_view(in vec3 z_axis) {
  // Be sure Y & Z are not colinears.
  vec3 y_axis = abs(z_axis.y) < (1.0 - Epsilon()) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
  vec3 x_axis = normalize(cross(y_axis, z_axis));
       y_axis = normalize(cross(z_axis, x_axis));
       z_axis = /*normalize*/(z_axis);
  return mat3(x_axis, y_axis, z_axis); 
}

// ----------------------------------------------------------------------------

vec2 trig_angle(float radians) {
  return vec2( cos(radians), sin(radians));
}

vec2 update_trig_angle(in vec2 base, in vec2 delta, float s) {
  return vec2(
    base.x * delta.x - s * base.y * delta.y,  // cos(a +/- b) = cos(a)*cos(b) -/+ sin(a)*sin(b)
    base.y * delta.x + s * base.x * delta.y   // sin(a +/- b) = sin(a)*cos(b) +/- cos(a)*sin(b) 
  );
}

vec2 inc_trig_angle(in vec2 base, in vec2 delta) {
  return update_trig_angle(base, delta, +1.0);
}

vec2 dec_trig_angle(in vec2 base, in vec2 delta) {
  return update_trig_angle(base, delta, -1.0);
}

// ----------------------------------------------------------------------------

float triangle_area(in vec3 A, in vec3 B, in vec3 C) {
  const vec3 AB = B - A;
  const vec3 AC = C - A;
  const float lenCP = sqrt(dot(AC, AC) - dot(AC, AB));
  return 0.5 * lenCP * length(AB);
}

// ----------------------------------------------------------------------------

// Generate stochastic coordinates in a triangle.
// ref. Greg Turk, "Graphics Gems", Academic Press, 1990
vec4 sample_triangle(in vec4 p0, in vec4 p1, in vec4 p2, vec2 st) {
  st.y = sqrt(st.y);
  vec3 coords;
  coords.x = 1.0 - st.y;
  coords.y = (1.0 - st.x) * st.y;
  coords.z = st.x * st.y;
  return mat3x4(p0, p1, p2) * coords;
}

// faster alternative.
vec4 sample_triangle2(in vec4 p0, in vec4 p1, in vec4 p2, vec2 st) {
  if ((st.x + st.y) > 1.0) {
    st.x = max(st.x, st.y);
    st.y = min(st.x, st.y);
    st.x = 1.0 - st.x;
  }
  vec3 coords = vec3(st.xy, 1.0 - (st.x + st.y));
  return mat3x4(p0, p1, p2) * coords;
}

// ----------------------------------------------------------------------------

// Sample a disk from 2 stochastic values.
vec3 sample_disk(float radius, vec2 rn) {
  const float r = radius * rn.x;
  const float theta = TwoPi() * rn.y;
  return vec3(r * cos(theta), 0.0, r * sin(theta));
}

// ----------------------------------------------------------------------------

// Sample a disk evenly for a given samples size.
// ref : http://blog.marmakoide.org/?p=1
vec3 sample_disk_even(float radius, uint id, uint total) {
  const float theta = id * GoldenAngle();
  const float r = radius *  sqrt(id / float(total));
  return r * vec3(cos(theta), 0, sin(theta));
}

// ----------------------------------------------------------------------------

// Sample a ball (interior & surface of a sphere) from 3 stochastic values.
// ref : so@5408276
vec3 sample_sphere_in(float radius, vec3 rn) {
  const float costheta = 2.0 * rn.x - 1.0;
  const float phi = TwoPi() * rn.y;
  const float theta = acos(costheta);
  const float r = radius * cbrt(rn.z);
  const float s = sin(theta);
  return r * vec3(s * cos(phi), s * sin(phi), costheta);
}

// ----------------------------------------------------------------------------

// Sample a sphere surface from 2 stochastic values.
// ref : https://www.cs.cmu.edu/~mws/rpos.html
//       https://gist.github.com/dinob0t/9597525 
vec3 sample_sphere_out(float radius, vec2 rn) {
  const float theta = TwoPi() * rn.x;
  const float z = radius * (2.0 * rn.y - 1.0);
  const float r = sqrt(radius * radius - z * z);
  return vec3(r * cos(theta), r * sin(theta), z);
}

// ----------------------------------------------------------------------------

// Samples points on an hemisphere using the Hammersley point set.
// ref : http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

float radicalInverse_VdC(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley2d(int i, float inv_N) {
  return vec2(float(i) * inv_N, radicalInverse_VdC(i));
}

vec2 hammersley2d(int i, int N) {
  return hammersley2d(i, 1.0f / float(N));
}

vec3 sample_hemisphere_uniform(float u, float v) {
 const float phi = v * TwoPi();
 const float cosTheta = 1.0 - u;
 const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
 return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
  
vec3 sample_hemisphere_cos(float u, float v) {
 const float phi = v * TwoPi();
 const float cosTheta = sqrt(1.0 - u);
 const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
 return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// ----------------------------------------------------------------------------

// Sample an hemisphere using Hammersley pointset on a roughness dependant specular lobe
// and return it as a direction vector in world space.
vec3 importance_sample_GGX( in mat3 basis_ws, in vec2 pt, float roughness_sqr) {
  pt.x = 1.0 - (1.0 - pt.x) / (1.0 + (roughness_sqr - 1.0) * pt.x);
  return normalize(basis_ws * sample_hemisphere_cos( pt.x, pt.y ));
}

// ----------------------------------------------------------------------------

vec4 hermite_mix(in vec3 p0, in vec3 p1, in vec3 t0, in vec3 t1, in float u) {
  const mat4 mHermit = mat4(
    vec4( 2.0, -3.0, 0.0, 1.0),
    vec4(-2.0,  3.0, 0.0, 0.0),
    vec4( 1.0, -2.0, 1.0, 0.0),
    vec4( 1.0, -1.0, 0.0, 0.0)
  );  
  
  vec4 vU = vec4(u*u*u, u*u, u, 1.0);

  mat4 B = mat4( 
    vec4(p0.x, p1.x, t0.x, t1.x),
    vec4(p0.y, p1.y, t0.y, t1.y),
    vec4(p0.z, p1.z, t0.z, t1.z),
    vec4(1.0, 1.0, 0.0, 0.0)
  );

  return vU * mHermit * B;
}

// ----------------------------------------------------------------------------

float saturate(float x) {
  return min(max(x, 0.0), 1.0);
}

// ----------------------------------------------------------------------------

// Map a range from [edge0, edge1] to [0, 1].
float maprange(float edge0, float edge1, float x) {
  return clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
}

// ----------------------------------------------------------------------------

// Map a value to smoothly peak at edge in [0, 1].
float smoothcurve(float edge, float x) {
  return smoothstep(0, edge, x) - smoothstep(edge, 1, x);
}

// ----------------------------------------------------------------------------

// float curve_inout(in float x, in float edge) {
//   const float a = maprange(0.0, edge, x);
//   const float b = maprange(edge, 1.0, x);
//   const float easein  = a * (2.0 - a);         // a * a;
//   const float easeout = b*b - 2.0 * b + 1.0;  // 1.0 - b * b;
//   return mix(easein, easeout, step(edge, x));
// }

// ----------------------------------------------------------------------------

// Higher order smoothstep.
float smoothstep2(float edge0, float edge1, float x) {
  const float t = maprange(edge0, edge1, x);
  return t * t * t * (10.0 + t *(-15.0 + 6.0 * t));
}

// ----------------------------------------------------------------------------

float gaussian(float sigma, float x_mu) {
  return exp(-(x_mu * x_mu) / (2.0 * sigma * sigma)) / (2.5066282 * abs(sigma));
}

vec3 gaussian(vec3 sigma, vec3 x_mu) {
  return exp(-(x_mu * x_mu) / (2.0 * sigma * sigma)) / (2.5066282 * abs(sigma));
}

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_INC_MATHS_GLSL_