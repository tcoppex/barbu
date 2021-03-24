#version 330 core

#include "im3d/inc_common.glsl"

// ----------------------------------------------------------------------------

in VertexData vData;

layout(location=0) out vec4 fResult;

// ----------------------------------------------------------------------------

void main() {
  fResult = vData.m_color;
  float d = abs(vData.m_edgeDistance) / vData.m_size;
  d = smoothstep(1.0, 1.0 - (kAntialiasing / vData.m_size), d);
  fResult.a *= d;
}

// ----------------------------------------------------------------------------