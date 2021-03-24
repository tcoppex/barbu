#version 330 core

#include "im3d/inc_common.glsl"

// ----------------------------------------------------------------------------

uniform mat4 uViewProjMatrix;

layout(location=0) in vec4 aPositionSize;
layout(location=1) in vec4 aColor;

out VertexData vData;

// ----------------------------------------------------------------------------

void main() {
  vData.m_color = aColor.abgr; // swizzle to correct endianness
  vData.m_color.a *= smoothstep(0.0, 1.0, aPositionSize.w / kAntialiasing);
  vData.m_size = max(aPositionSize.w, kAntialiasing);
  gl_Position = uViewProjMatrix * vec4(aPositionSize.xyz, 1.0);  
  gl_PointSize = vData.m_size;
}

// ----------------------------------------------------------------------------