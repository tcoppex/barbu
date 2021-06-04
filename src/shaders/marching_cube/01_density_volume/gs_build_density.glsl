#version 430 core

// ----------------------------------------------------------------------------

// The Geometry Shader sole purpose is to set the destination layer
// (via gl_Layer) for the 3D texture.

in block {
  vec4 positionCS;
  vec3 coordWS;
  int instanceID;
} In[];

out block {
  vec3 coordWS;
} Out;

// ----------------------------------------------------------------------------

#define NVERTICES   3

layout(triangles) in;
layout(triangle_strip, max_vertices = NVERTICES) out;

void main() {
  // Per primitive attributes
  gl_Layer = In[0].instanceID;
  
  // Per vertex attributes
  for (int i = 0; i < NVERTICES; ++i) {
    gl_Position = In[i].positionCS;
    Out.coordWS = In[i].coordWS;
    EmitVertex();
  }
  EndPrimitive();
}

// ----------------------------------------------------------------------------