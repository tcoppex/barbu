#version 430 core

//------------------------------------------------------------------------------

in block {
  vec3 v1, n1;
  vec3 v2, n2;
  vec3 v3, n3;
} In[];

out vec3 outPositionWS;
out vec3 outNormalWS;

//------------------------------------------------------------------------------

layout(points) in;
layout(points, max_vertices = 3) out;

void main() {
  outPositionWS = In[0].v1;
  outNormalWS   = In[0].n1;
  EmitVertex();
  
  outPositionWS = In[0].v2;
  outNormalWS   = In[0].n2;
  EmitVertex();
  
  outPositionWS = In[0].v3;
  outNormalWS   = In[0].n3;
  EmitVertex();
  
  EndPrimitive();
}

//------------------------------------------------------------------------------
