#version 430 core

#include "hair/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = BINDING_HAIR_RENDER_ATTRIB_IN) in vec4 position_xyz_coeff_w;

// Outputs.
layout(location = 0) out vec3 outPosition;
layout(location = 1) out float outCoeff;

// ----------------------------------------------------------------------------

void main() {
  outPosition   = position_xyz_coeff_w.xyz;
  outCoeff      = position_xyz_coeff_w.w;
}

// ----------------------------------------------------------------------------