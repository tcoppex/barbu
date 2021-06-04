#version 430 core

in block {
  ivec3 coords_case_numTri;
} In[];

out uint x6y6z6_e4e4e4;

// size: 1280 * short [3 x 4bits]
uniform isamplerBuffer uEdgeConnectList;

// ----------------------------------------------------------------------------

#define NVERTICES   5

layout(points) in;
layout(points, max_vertices = NVERTICES) out;

void main() {
  const int PackedCoords = In[0u].coords_case_numTri.x;
  const int CubeCase     = In[0u].coords_case_numTri.y;
  const int NumTri       = In[0u].coords_case_numTri.z;

  for (int i = 0; i < NumTri; ++i) {
    const int offset = NVERTICES * CubeCase + i;
    const int packed_edges = texelFetch(uEdgeConnectList, offset).r;

    x6y6z6_e4e4e4 = uint(PackedCoords) | uint(packed_edges << 18);
    EmitVertex();
  }
  EndPrimitive();
}

// ----------------------------------------------------------------------------