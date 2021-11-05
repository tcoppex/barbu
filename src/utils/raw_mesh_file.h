#ifndef BARBU_UTILS_RAW_MESH_FILE_H_
#define BARBU_UTILS_RAW_MESH_FILE_H_

// ----------------------------------------------------------------------------
// Basic data structure for loading meshes with sparse attributes.
// ----------------------------------------------------------------------------

#include <string>
#include <vector>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

// ----------------------------------------------------------------------------

// Vertex Group.
// Defines a subpart of a mesh, represented by a range of vertex indices.
struct VertexGroup {
  std::string name;
  int32_t start_index = -1;
  int32_t end_index = -1;
  
  int32_t nelems() const { 
    return end_index - start_index;
  }
};

using VertexGroups_t = std::vector<VertexGroup>;

// ----------------------------------------------------------------------------

// RawMesh.
// Structure of sparse attributes arrays (SoA-layout) used to built a MeshData
// or import one from an external source.
struct RawMeshData {
  // Mesh name, could be empty.
  std::string name;

  // List of unique sparse vertex attributes.
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> texcoords;
  std::vector<glm::vec3> normals;

  // Tangents are indexed as elementsAttribs. 
  // (the W component contains the sign of the binormal)
  std::vector<glm::vec4> tangents; //

  std::vector<glm::uvec4> joints;
  std::vector<glm::vec4> weights;

  // List of triangle faces, each elements is a vertex containing indices to its attribute.
  // (respectively position, texcoord, normals). Contains 3 * nfaces elements.
  std::vector<glm::ivec3> elementsAttribs;

  // Sub-geometry description.
  VertexGroups_t vgroups;  

  static constexpr size_t kDefaultTriangleCapacity = 512;
  static constexpr size_t kDefaultCapacity = 3 * kDefaultTriangleCapacity;
  
  RawMeshData(size_t capacity = kDefaultCapacity) {
    vertices.reserve(capacity);
    texcoords.reserve(capacity);
    normals.reserve(capacity);
    tangents.reserve(capacity); //
    elementsAttribs.reserve(capacity);
  }
  
  inline void addVertex(glm::vec3 const& v) {
    vertices.push_back(v);
  }
  
  inline void addTexcoord(glm::vec3 const& v) {
    texcoords.push_back(v);
  }

  inline void addNormal(glm::vec3 const& v) {
    normals.push_back(v);
  }

  inline void addIndex(int index) {
    elementsAttribs.push_back(glm::ivec3(index));
  }
  
  inline int32_t nfaces() const { 
    return static_cast<int32_t>(elementsAttribs.size()/3); // 
  }

  // inline int32_t nvertices() const {
  //   return static_cast<int32_t>(vertices.size()); //
  // }

  bool hasVertexGroups() const { 
    return !vgroups.empty();
  }

  void reserveSkinningData() {
    joints.reserve(vertices.capacity());
    weights.reserve(vertices.capacity());
  }
  
  // Use elementsAttribs & vertices to fill the normals attributes.
  void recalculateNormals();

  // Calculate tangent space based on vertices position, texcoords & normals,
  // and index them on elementsAttribs.
  void recalculateTangents(); //
};

// ----------------------------------------------------------------------------

// Raw material info used to construct a MaterialAsset.
struct MaterialInfo {
  std::string name;

  std::string diffuse_map;
  std::string specular_map;
  std::string emissive_map;
  std::string metallic_rough_map;
  std::string bump_map; //
  std::string ao_map;
  std::string alpha_map;

  glm::vec4 diffuse_color{ 1.0f, 1.0f, 1.0f, 0.75f };      // RGB Albedo + Alpha
  glm::vec4 specular_color{ 0.0f, 0.0f, 0.0f, 1.0f };      // XYZ specular + W Exponent
  glm::vec3 emissive_factor{ 0.0f, 0.0f, 0.0f }; //

  float metallic     = 0.0f;
  float roughness    = 0.4f;
  float alpha_cutoff = 0.5f;

  bool bAlphaTest   = false;
  bool bBlending    = false;  // (Blending always preempt alpha test)
  bool bDoubleSided = true; //
  bool bUnlit       = false;
};

// MaterialFile.
// Defines a set of material infos from a file.
struct MaterialFile {
  std::string id;
  std::vector<MaterialInfo> infos;

  inline MaterialInfo const& get(size_t index) const {
    return infos.at(index);
  }

  // bool has_normal_map() const {
  //   for (auto const& mat : infos) {
  //     if (!mat.bump_map.empty()) {
  //       return true;
  //     }
  //   }
  //   return false;
  // }
};

// ----------------------------------------------------------------------------

// RawMeshFile.
// Represents a list of meshes within a single file possibly sharing a common material file.
struct RawMeshFile {
  std::string material_id;            // Material file id / relative path name.
  std::vector<RawMeshData> meshes;    // Buffer of meshes.

  static constexpr size_t kDefaultCapacity = 4;
  RawMeshFile(size_t capacity = kDefaultCapacity) {
    meshes.reserve(capacity);
  }

  // [to clarify]
  // Prefix VertexGroups and materials names by the main material id to avoid collisions.
  void prefixMaterialVertexGroupNames(MaterialFile &mtl) {
    for (auto &mesh : meshes) {
      for (auto &vg : mesh.vgroups) {
        vg.name = mtl.id + "::" + vg.name;
      }
    }
    for (auto &mat : mtl.infos) {
      mat.name = mtl.id + "::" + mat.name;
      //LOG_INFO(mat.name);
    }
  }
};

// ----------------------------------------------------------------------------

#endif // BARBU_UTILS_RAW_MESH_FILE_H_