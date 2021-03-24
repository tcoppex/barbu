#ifndef SHADERS_INC_SOLVER_GLSL_
#define SHADERS_INC_SOLVER_GLSL_

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------
// Polynomial solvers up to three degrees.
//
// All solvers return a vec4 where : 
//     - XYZ are the individuals roots,
//     - W is the number of roots.
//
// ----------------------------------------------------------------------------

vec4 solver_linear(float a, float b) {
  return (abs(a) > Epsilon()) ? vec4(-b / a, 0, 0, 1) : vec4(0);
}

// ----------------------------------------------------------------------------

vec4 solver_quadratic(float a, float b, float c) {
  if (abs(a) < Epsilon()) {
    return solver_linear(b, c);
  }
  float delta = (b * b - 4 * a * c);
  if (delta < 0.0) {
    return vec4(0);
  }
  delta = sqrt(delta);
  const float r1 = (- b + delta) / (2 * a);
  const float r2 = (- b - delta) / (2 * a);
  return vec4(r1, r2, 0, 1 + step(Epsilon(), delta));
}

// ----------------------------------------------------------------------------

vec4 solver_cubic_normalized(float a, float b, float c);
vec4 solver_cubic_fast(float a, float b, float c, float d);

vec4 solver_cubic(float a, float b, float c, float d) {
  return (abs(a) < Epsilon()) ? solver_quadratic(b, c, d)
                              : solver_cubic_normalized(b/a, c/a, d/a)
                              //: solver_cubic_fast(a, b, c, d)
                              ;
}

// -----------------------------------------------------------------------------

vec4 solver_cubic_normalized(float a, float b, float c) {
  vec4 roots = vec4(0.0);

  if (abs(c) < Epsilon())
  {
    roots = solver_quadratic(1.0, a, b);
    roots[int(roots.w)] = 0.0;
    roots.w += 1.0;
  }
  else
  {
    const float Q = (3 * b - a * a) / 9;
    const float R = (9 * a * b - 27 * c - 2 * a * a * a) / 54;
    const float Q3 = Q * Q * Q;
    const float D = Q3 + R * R;
    const float third_a = a / 3;

    if (D > 0)
    {
      const float sqrtD = sqrt(D);
      const float s = float(sign(R + sqrtD)) * pow(abs(R + sqrtD), 0.333f);
      const float t = float(sign(R - sqrtD)) * pow(abs(R - sqrtD), 0.333f);
      roots.x = s + t - third_a;
      roots.w = 1;
    }
    else
    {
      const float theta = acos(R * inversesqrt(-Q3));
      const float twoSqrtQ = 2.0f * sqrt(-Q);
      roots.x = cos(theta / 3);
      roots.y = cos((theta + 2 * Pi()) / 3);
      roots.z = cos((theta + 4 * Pi()) / 3);
      roots.xyz = fma(vec3(twoSqrtQ), roots.xyz, vec3(-third_a));
      roots.w = 3;
    }
  }

  return roots;
}

// ----------------------------------------------------------------------------

// ref : http://momentsingraphics.de/CubicRoots.html
// vec4 solver_cubic_fast(float a, float b, float c, float d) {
//   // Normalize coefficients.
//   a /= a;
//   b /= a;
//   c /= a;
//   d /= a;

//   // Divide middle coefficients by three.
//   b /= 3;
//   c /= 3;

//   // Compute the Hessian and the discrimant.
//   const vec3 delta = vec3(
//     fma(-c, c, b),
//     fma(-b, c, a),
//     a * c - b * b  
//   );
//   const float D = 4.0 * a * c - b * b;

//   // Compute coefficients of the depressed cubic 
//   // (third is zero, fourth is one)
//   const vec2 depressed = vec2(
//     fma(-2*c, delta.x, delta.y),
//     delta.x
//   );

//   // Take the cubic root of a normalized complex number
//   const float theta = atan(sqrt(D),-depressed.x) / 3.0f;
//   const vec2 cubicRoots = vec2( cos(theta), sin(theta) );
//   const vec3 scale = 0.5 * vec3( -1.0, -1.7320508, 1.7320508);
  
//   // Compute the three roots, scale appropriately and 
//   // revert the depression transform.
//   vec3 roots = vec3(
//     cubicRoots.x,
//     dot( scale.xy, cubicRoots),
//     dot( scale.xz, cubicRoots)
//   );
//   roots = fma( vec3(2.0 * sqrt(-depressed.y)), roots, vec3(-c));

//   return vec4( roots, 3);
// }

// ----------------------------------------------------------------------------

vec4 solver_polynomial(in vec4 coeffs) {
  return solver_cubic(coeffs.x, coeffs.y, coeffs.z, coeffs.w);
}

float polynomial_result(in vec4 coeffs, float x) {
  return dot(coeffs, vec4(pow(x, 3), pow(x, 2), x, 1.0));
}

vec4 derivate_coefficients(in vec4 coeffs) {
  return vec4(0, 3*coeffs.x, 2*coeffs.y, coeffs.z);
}

// ----------------------------------------------------------------------------

#endif // SHADERS_INC_SOLVER_GLSL_
