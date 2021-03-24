#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec3 inTangent[];
layout(location = 2) in int inInstanceID[];
layout(location = 3) in float inRelativePos[];

// Outputs.
layout(vertices = 6) out;

layout(location = 0) out vec3 outPosition[];
layout(location = 1) out vec3 outTangent[];
layout(location = 2) invariant patch out int outInstanceID;
layout(location = 3) invariant patch out float outRelativePos[];

// Uniforms.
uniform int uNumLines       = 1;
uniform int uNumSubSegments = 1;

uniform float uScaleFactor;

// ----------------------------------------------------------------------------

// [could be bypassed if outInstanceID was not used]

void main() {
#define ID  gl_InvocationID
  
  // Tesselation parameters.
  if (0 == ID) {
    gl_TessLevelOuter[0] = uNumLines;
    gl_TessLevelOuter[1] = uNumSubSegments;
    outInstanceID = inInstanceID[0];
    outRelativePos[ID] = inRelativePos[0];
  }

  outPosition[ID] = inPosition[ID];
  outTangent[ID]  = inTangent[ID] * uScaleFactor;
}

// ----------------------------------------------------------------------------
