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

// Note : As TES cannot output lines_adjacency, hair strands cannot be transformed
//        as tristrip ribbons through the following geometry stage
//        (cf. https://www.khronos.org/opengl/wiki/Geometry_Shader).
//
//        Instead we use transform feedback to save the tesselated strands 
//        linestrip into a buffer in one pass and render them with a lines_adjacency 
//        GS in a second.

// (should probably use two stream : one for the vertices, another for the indices..)

void main() {
  position_xyz_coeff_w = vec4( inPosition[0].xyzw /*, inColorCoeff[0]*/ ); EmitVertex();
  position_xyz_coeff_w = vec4( inPosition[1].xyzw /*, inColorCoeff[1]*/ ); EmitVertex();
  EndPrimitive();
}

// ----------------------------------------------------------------------------
