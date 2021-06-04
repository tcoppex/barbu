#version 430 core

// ----------------------------------------------------------------------------

// (original)
//layout(location = 0) in vec4 inPositionUV;

#include "generic/interop.h"
layout(location = VERTEX_ATTRIB_POSITION)  in vec3 inPosition;
layout(location = VERTEX_ATTRIB_TEXCOORD)  in vec2 inTexcoord;

out block {
  vec4 positionCS;
  smooth vec3 coordWS;
  int instanceID;
} Out;

uniform vec3  uChunkPositionWS;
uniform float uChunkSizeWS;           // world-space size of a chunk
uniform float uInvChunkDim;
uniform float uScaleCoord;            //uTextureRes * uInvWindowDim
uniform float uTexelSize;             //1.0f / uTextureRes;
uniform float uMargin;
uniform float uWindowDim;

// ----------------------------------------------------------------------------

void main() {
# define Position  inPosition.xz
# define TexCoord  inTexcoord.xy

  // in [0, 1]
  vec3 chunkcoord = vec3(TexCoord, gl_InstanceID * uTexelSize);
       chunkcoord *= uScaleCoord;

  // Change chunkcoord interval to calculate texels in the marge.
  chunkcoord = (chunkcoord * uWindowDim - uMargin) * uInvChunkDim;

  const vec3 ws = uChunkPositionWS + uChunkSizeWS * chunkcoord;

  Out.positionCS  = vec4(Position, 0.0f, 1.0f); // OpenGL clip-space
  Out.coordWS     = ws;
  Out.instanceID  = gl_InstanceID;
}

// ----------------------------------------------------------------------------
