#version 430 core

#include "generic/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = VERTEX_ATTRIB_POSITION) in vec3 inPosition;
layout(location = VERTEX_ATTRIB_TEXCOORD) in vec2 inTexcoord;
layout(location = VERTEX_ATTRIB_NORMAL) in vec3 inNormal;

layout(location = VERTEX_ATTRIB_JOINT_INDICES) in uvec4 inJointIndices; // << BUG ??
layout(location = VERTEX_ATTRIB_JOINT_WEIGHTS) in vec4 inJointWeights;

// Outputs.
layout(location = 0) out vec3 outPositionWS;
layout(location = 1) out vec2 outTexcoord;
layout(location = 2) out vec3 outNormalWS;
// layout(location = 3) out vec4 outDebugColor;

// ----------------------------------------------------------------------------

#include "shared/inc_skinning.glsl"

// Uniforms [ use uniform block ].
uniform mat4 uMVP;
uniform mat4 uModelMatrix;

// ----------------------------------------------------------------------------

void main() {
  vec4 position = vec4(inPosition, 1.0);
  vec3 normal = inNormal;

  apply_skinning( inJointIndices, inJointWeights, position.xyz, normal); //
  
  gl_Position   = uMVP * position;
  outPositionWS = vec3(uModelMatrix * position);
  outTexcoord   = inTexcoord;
  outNormalWS   = normalize(mat3(uModelMatrix) * normal);
}

// ----------------------------------------------------------------------------