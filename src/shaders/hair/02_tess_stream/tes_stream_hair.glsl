#version 430 core

#include "hair/interop.h"
#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------

// Inputs.
layout(isolines) in;
layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec3 inTangent[];
layout(location = 2) patch in int inInstanceID;
layout(location = 3) patch in float inRelativePos[];

// Outputs.
layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outTangent;

// Uniforms.
uniform int uNumInstances;

layout(std430, binding = SSBO_HAIR_TF_RANDOMBUFFER)
readonly buffer RandomBuffer {
  vec3 randbuffer[];
};

// ----------------------------------------------------------------------------

void main() {
  // for each line strip :
  //   * gl_TessCoord.x is in range [0, 1[,
  //   * gl_TessCoord.y is constant.
  const float dx = gl_TessCoord.x;

  // Interpolated point at each vertex.
#if 1
  const vec4 p0 = hermite_mix(inPosition[0], inPosition[1], inTangent[0], inTangent[1], dx);
  const vec4 p1 = hermite_mix(inPosition[2], inPosition[3], inTangent[2], inTangent[3], dx);
  const vec4 p2 = hermite_mix(inPosition[4], inPosition[5], inTangent[4], inTangent[5], dx);
#else
  // (using this, styles should be computed via angular constraints).
  const vec4 p0 = vec4(mix(inPosition[0], inPosition[1], dx), 1.0);
  const vec4 p1 = vec4(mix(inPosition[2], inPosition[3], dx), 1.0);
  const vec4 p2 = vec4(mix(inPosition[4], inPosition[5], dx), 1.0);
#endif
  // ..
  const vec4 t0 = vec4(mix(inTangent[0], inTangent[1], dx), 0.0);
  const vec4 t1 = vec4(mix(inTangent[2], inTangent[3], dx), 0.0);
  const vec4 t2 = vec4(mix(inTangent[4], inTangent[5], dx), 0.0);

  // Random texture coordinates. [to clean / "fix"]
  const vec3 randCoords = randbuffer[
    int(gl_TessCoord.y*40 + inInstanceID) % HAIR_TF_RANDOMBUFFER_SIZE //
  ];

  // (alternative via a 1D RGB16f texture)
  // const float texelSize = 1.0f / (textureSize(uRandomTexture, 0).x - 1.0f);
  // const float texcoord = gl_TessCoord.y + inInstanceID * texelSize;
  // const vec3 randCoords = texture(uRandomTexture, texcoord).xyz;

  // Interpolate position via random point sampling.
  const vec4 position = sample_triangle2(p0, p1, p2, randCoords.xy);
  const vec4 tangent  = sample_triangle2(t0, t1, t2, randCoords.xy);

  float relPos = inRelativePos[0] + gl_TessCoord.x / float(HAIR_MAX_PARTICLE_PER_STRAND); //
        relPos = smoothstep2(0.0, 1.0, relPos);

  outPosition  = vec4( position.xyz, relPos);

  // [should output a per-strand coeff as well as the tangent / normal]
  //outColorCoeff = float(inInstanceID + 1.0f) / float(uNumInstances);
  outTangent   = vec4( tangent.xyz, 0); //
}

// ----------------------------------------------------------------------------
