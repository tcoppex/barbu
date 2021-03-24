#version 330 core

#include "im3d/inc_common.glsl"

// ----------------------------------------------------------------------------

in VertexData vData;

layout(location=0) out vec4 fResult;

// ----------------------------------------------------------------------------

void main() {
  fResult = vData.m_color;
  float d = length(gl_PointCoord.xy - vec2(0.5));
  d = smoothstep(0.5, 0.5 - (kAntialiasing / vData.m_size), d);
  fResult.a *= d;
}

// ----------------------------------------------------------------------------