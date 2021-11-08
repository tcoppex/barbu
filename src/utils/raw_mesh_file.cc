#include "utils/raw_mesh_file.h"

#include <cstring>  // memcpy
#include "glm/glm.hpp" // glm::normalize
#include "MikkTSpace/mikktspace.h"

#include "core/logger.h"

// ----------------------------------------------------------------------------

void RawMeshData::recalculateNormals() {
  int const numfaces = nfaces();
  std::vector<glm::vec3> vnormals( vertices.size(), glm::vec3(0.0f));
  
  for (int i = 0; i < numfaces; ++i) {
    int const i1 = elementsAttribs[3*i + 0].x;
    int const i2 = elementsAttribs[3*i + 1].x;
    int const i3 = elementsAttribs[3*i + 2].x;

    auto const& v1 = vertices[i1];
    auto const& v2 = vertices[i2];
    auto const& v3 = vertices[i3];

    glm::vec3 const u{ v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
    glm::vec3 const v{ v3.x - v2.x, v3.y - v2.y, v3.z - v2.z };
    glm::vec3 const n{ glm::normalize(glm::cross(u, v)) };

    vnormals[i1] += n;
    vnormals[i2] += n;
    vnormals[i3] += n;
  }

  for (auto &n : vnormals) {
    n = glm::normalize(n);
  }

  normals.clear();
  for (int i = 0; i < numfaces; ++i) {
    for (int j = 0; j < 3; ++j) {
      auto& face = elementsAttribs[3*i + j];
      int const vertex_id = face.x;
      auto const& vnormal = vnormals[vertex_id];

      // [ use a hashmap to avoid reusing the same normals ? ]
      face.z = static_cast<uint32_t>(normals.size());
      normals.push_back( glm::vec3(vnormal.x, vnormal.y, vnormal.z) );
    }
  }
}

void RawMeshData::recalculateTangents() {
  assert(!vertices.empty());

  // -- More informations on http://www.mikktspace.com/ --

  if (normals.empty()) {
    LOG_DEBUG_INFO( "Tangent space not calculated : missing normals." );
    return;    
  }

  if (texcoords.empty()) {
    LOG_DEBUG_INFO( "Tangent space not calculated : missing texcoords." );
    return;    
  }


  SMikkTSpaceInterface interface;

  interface.m_getNumFaces = [](const SMikkTSpaceContext * pContext) {
    auto &self = *((RawMeshData*)pContext->m_pUserData); 
    return self.nfaces();
  };

  interface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext * pContext, const int iFace) {
    return 3;
  };

  interface.m_getPosition = [](const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert) {
    auto &self = *((RawMeshData*)pContext->m_pUserData);
    auto const vid = self.elementsAttribs[3*iFace + iVert].x;
    auto const& v = self.vertices[vid];
    fvPosOut[0] = v.x;
    fvPosOut[1] = v.y;
    fvPosOut[2] = v.z;
  };

  interface.m_getNormal = [](const SMikkTSpaceContext * pContext, float fvNormOut[], const int iFace, const int iVert) {
    auto &self = *((RawMeshData*)pContext->m_pUserData);
    auto const nid = self.elementsAttribs[3*iFace + iVert].z;
    auto const& n = self.normals[nid];
    fvNormOut[0] = n.x;
    fvNormOut[1] = n.y;
    fvNormOut[2] = n.z;
  };

  interface.m_getTexCoord = [](const SMikkTSpaceContext * pContext, float fvTexcOut[], const int iFace, const int iVert) {
    auto &self = *((RawMeshData*)pContext->m_pUserData);
    auto const tid = self.elementsAttribs[3*iFace + iVert].y;
    auto const& t = self.texcoords[tid];
    fvTexcOut[0] = t.x;
    fvTexcOut[1] = t.y;
    //LOG_MESSAGE(t.x, t.y);
  };
  
  interface.m_setTSpaceBasic = [](const SMikkTSpaceContext * pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
    auto &self = *((RawMeshData*)pContext->m_pUserData);
    int32_t const index = 3 * iFace + iVert;
    self.tangents[index] = glm::vec4(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
  };

  interface.m_setTSpace = [](const SMikkTSpaceContext * pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS, const float fMagT,
            const tbool bIsOrientationPreserving, const int iFace, const int iVert) {
    //LOG_ERROR( __FUNCTION__, "not implemented." );
  };


  tangents.resize( elementsAttribs.size() ); //
  SMikkTSpaceContext context{ &interface, this };
  genTangSpaceDefault( &context );
}

// ----------------------------------------------------------------------------