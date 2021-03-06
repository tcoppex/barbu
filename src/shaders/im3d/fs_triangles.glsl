#version 330 core

#include "im3d/inc_common.glsl"

// ----------------------------------------------------------------------------

in VertexData vData;

layout(location=0) out vec4 fResult;

// ----------------------------------------------------------------------------

void main() {
  fResult = vData.m_color;
}

// ----------------------------------------------------------------------------