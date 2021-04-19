#version 430 core

#include "hair/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(lines, invocations = 1) in;
layout(location = 0) in vec4 inPosition[];
//layout(location = 1) in float inColorCoeff[];

// Outputs.
layout(line_strip, max_vertices = 2) out;
layout(location = BINDING_HAIR_TF_ATTRIB_OUT) out vec4 position_xyz_coeff_w;

// ----------------------------------------------------------------------------

// (should probably use two streams : one for vertices, another for indices)

void main() {
  position_xyz_coeff_w = inPosition[0].xyzw;
  EmitVertex();

  position_xyz_coeff_w = inPosition[1].xyzw;
  EmitVertex();
  
  EndPrimitive();
}

// ----------------------------------------------------------------------------
