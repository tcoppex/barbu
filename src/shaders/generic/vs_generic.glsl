#version 430 core

#include "generic/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = VERTEX_ATTRIB_POSITION)       in vec3 inPosition;
layout(location = VERTEX_ATTRIB_TEXCOORD)       in vec2 inTexcoord;
layout(location = VERTEX_ATTRIB_NORMAL)         in vec3 inNormal;
layout(location = VERTEX_ATTRIB_TANGENT)        in vec4 inTangent;
layout(location = VERTEX_ATTRIB_JOINT_INDICES)  in uvec4 inJointIndices;
layout(location = VERTEX_ATTRIB_JOINT_WEIGHTS)  in vec4 inJointWeights;

// Outputs.
layout(location = 0) out vec2 outTexcoord;
layout(location = 1) out vec3 outPositionWS;
layout(location = 2) out vec3 outNormalWS;
layout(location = 3) out vec4 outTangentWS;

// ----------------------------------------------------------------------------

#include "shared/inc_skinning.glsl"

// Uniforms.
uniform mat4 uModelMatrix;
uniform mat4 uMVP; // [use viewProj instead ?]

// ----------------------------------------------------------------------------

void main() {
  vec4 position = vec4(inPosition, 1.0);
  vec3 normal   = inNormal;
  vec3 tangent  = inTangent.xyz;

  // [should transform BTN too]
  apply_skinning(inJointIndices, inJointWeights, position.xyz, normal); //
  
  // Transform vectors.
  const mat3 normalMatrix = mat3(uModelMatrix);
  normal  = normalize(normalMatrix * normal);
  tangent = normalize(normalMatrix * tangent);

  // Outputs.
  gl_Position   = uMVP * position;
  outTexcoord   = inTexcoord;
  outPositionWS = (uModelMatrix * position).xyz;
  outNormalWS   = normal;
  outTangentWS  = vec4(tangent, inTangent.w);
}

// ----------------------------------------------------------------------------