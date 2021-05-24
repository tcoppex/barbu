#version 430 core

#include "generic/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = VERTEX_ATTRIB_POSITION) in vec3 inPosition;
layout(location = VERTEX_ATTRIB_TEXCOORD) in vec2 inTexcoord;
layout(location = VERTEX_ATTRIB_NORMAL) in vec3 inNormal;
layout(location = VERTEX_ATTRIB_TANGENT) in vec4 inTangent;
layout(location = VERTEX_ATTRIB_JOINT_INDICES) in uvec4 inJointIndices;
layout(location = VERTEX_ATTRIB_JOINT_WEIGHTS) in vec4 inJointWeights;

// Outputs.
layout(location = 0) out vec3 outPositionWS;
layout(location = 1) out vec2 outTexcoord;
layout(location = 2) out vec3 outNormalWS;
layout(location = 3) out vec4 outTangentWS;

// ----------------------------------------------------------------------------

#include "shared/inc_skinning.glsl"

// Uniforms [ use uniform block ].
uniform mat4 uMVP;
uniform mat4 uModelMatrix;

// ----------------------------------------------------------------------------

void main() {
  vec3 position = inPosition;
  vec3 normal   = inNormal;
  vec3 tangent  = inTangent.xyz;

  // [should transform tangent too]
  apply_skinning( inJointIndices, inJointWeights, position, normal); //
  
  const mat3 modelMatrix = mat3(uModelMatrix); //
  gl_Position   = uMVP * vec4(position, 1.0);
  outPositionWS = modelMatrix * position;
  outTexcoord   = inTexcoord;
  outNormalWS   = normalize(modelMatrix * normal);
  outTangentWS  = vec4(normalize(modelMatrix * tangent), inTangent.w);
}

// ----------------------------------------------------------------------------