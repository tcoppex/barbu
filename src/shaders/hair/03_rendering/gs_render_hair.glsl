#version 430 core

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------

// Inputs.
layout(lines, invocations = 1) in;
layout(location = 0) in vec3 inPosition[];
layout(location = 1) in float inCoeff[];

// Outputs.
layout(triangle_strip, max_vertices = 4) out;
layout(location = 0) out vec3 outPosition;

// Uniforms.
uniform mat4 uMVP;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uLineWidth;

// ----------------------------------------------------------------------------

mat3 basisVS(in mat4 viewMatrix, in vec3 p0, in vec3 p1) {
  const mat3 view3 = mat3(viewMatrix);


  // View space direction.
#if 0
  vec3 u = vec3(viewMatrix * vec4(p1 - p0, 1));
  vec3 v = cross(u, vec3(0,1,0));
#else
  // "incorrect" but give more interesting result imo

  vec3 u = view3 * (p1 - p0);
  u.z = 0.0;
  u = normalize(u);
  vec3 v = vec3( -u.y, u.x, 0.0);
#endif

  // Change of basis matrix.
  const vec3 a = normalize(u*view3);
  const vec3 b = normalize(v*view3);
  const vec3 c = normalize(-cross(a, b));
  return mat3(a, b, c);
}

// ----------------------------------------------------------------------------

void fetchVert(in mat4 mvp, in vec4 v) {
  gl_Position = mvp * v;
  outPosition = v.xyz;
  EmitVertex();
}

void EmitQuadVS(in vec3 A, in vec3 B, in vec3 Y0, in vec3 Y1) {
  fetchVert( uMVP, vec4(A + Y0, 1.0f));
  fetchVert( uMVP, vec4(A - Y0, 1.0f));
  fetchVert( uMVP, vec4(B + Y1, 1.0f));
  fetchVert( uMVP, vec4(B - Y1, 1.0f));

  EndPrimitive();
}


// ----------------------------------------------------------------------------

void main() {

  // (attached on view space)
  const vec3 side = vec3(
    uLineWidth * (1.0 - inCoeff[0]),
    uLineWidth * (1.0 - inCoeff[1]),
    0
  );

  const mat3 Ybasis = basisVS( uView, inPosition[0], inPosition[1]);
  EmitQuadVS( inPosition[0].xyz, inPosition[1].xyz, Ybasis * side.zxz, Ybasis * side.zyz);

}

// ----------------------------------------------------------------------------
