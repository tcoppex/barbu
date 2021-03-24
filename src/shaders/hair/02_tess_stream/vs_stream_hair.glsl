#version 430 core

#include "hair/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = SSBO_HAIR_SIM_POSITION_READ) in vec4 inPosition;
layout(location = SSBO_HAIR_SIM_TANGENT_READ) in vec4 inTangent;

// Outputs.
layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outTangent;
layout(location = 2) out invariant int outInstanceID;
layout(location = 3) out invariant float outRelativePos;

// ----------------------------------------------------------------------------

void main() {
  outPosition   = inPosition.xyz;
  outTangent    = inTangent.xyz;
  outInstanceID = gl_InstanceID;

  outRelativePos = (gl_VertexID % HAIR_MAX_PARTICLE_PER_STRAND) / float(HAIR_MAX_PARTICLE_PER_STRAND); //
}

// ----------------------------------------------------------------------------