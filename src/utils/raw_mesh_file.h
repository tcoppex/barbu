#ifndef BARBU_UTILS_RAW_MESH_FILE_H_
#define BARBU_UTILS_RAW_MESH_FILE_H_

#include <string>
#include <vector>

#include "glm/glm.hpp" // glm::normalize
// #include "glm/vec2.hpp"
// #include "glm/vec3.hpp"
// #include "glm/vec4.hpp"

// ----------------------------------------------------------------------------

// Basic data structure for loading meshes with sparse attributes.

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
  std::vector<glm::uvec4> joints;
  std::vector<glm::vec4> weights;

  // List of triangle faces, each elements is a vertex containing indices to its attribute.
  // (respectively position, texcoord, normals). Contains 3 * nfaces elements.
  std::vector<glm::ivec3> elementsAttribs;

  // Sub-geometry description.
  VertexGroups_t vgroups;

  static constexpr size_t kDefaultTriangleCapacity = 256;
  static constexpr size_t kDefaultCapacity = 3 * kDefaultTriangleCapacity;
  RawMeshData(size_t capacity = kDefaultCapacity) {
    vertices.reserve(capacity);
    texcoords.reserve(capacity);
    normals.reserve(capacity);
    elementsAttribs.reserve(capacity);
  }

  // Use elementsAttribs & vertices to fill the normals attributes.
  void recalculate_normals() {
    int const numfaces = nfaces();
    std::vector<glm::vec3> vnormals(nvertices(), glm::vec3(0.0f));
    
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

        // TODO : use a hashmap to avoid reusing the same normals.
        face.z = static_cast<uint32_t>(normals.size());
        normals.push_back( glm::vec3(vnormal.x, vnormal.y, vnormal.z) );
      }
    }
  }

  inline int32_t nvertices() const { 
    return static_cast<int32_t>(vertices.size());
  }
  
  inline int32_t nfaces() const { 
    return elementsAttribs.empty() ? 0 : static_cast<int32_t>(elementsAttribs.size()/3); 
  }

  bool has_vertex_groups() const { 
    return !vgroups.empty();
  }

  void reserve_skinning() {
    joints.reserve(vertices.capacity());
    weights.reserve(vertices.capacity());
  }
};

// ----------------------------------------------------------------------------

// Raw material info used to construct a MaterialAsset.
struct MaterialInfo {
  std::string name;

  glm::vec4 diffuse_color{ 1.0f, 1.0f, 1.0f, 0.75f };      // RGB Albedo + Alpha
  glm::vec4 specular_color{ 0.0f, 0.0f, 0.0f, 1.0f };      // XYZ specular + W Exponent

  std::string diffuse_map;
  std::string metallic_rough_map;
  std::string bump_map;
  std::string alpha_map;
  std::string specular_map;

  float alpha_cutoff = 0.5f;

  float metallic  = 0.0f;
  float roughness = 0.4f;

  bool bAlphaTest   = false;
  bool bBlending    = false;  // (Blending always preempt alpha test)

  bool bDoubleSided = false;
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
  void prefix_material_vg_names(MaterialFile &mtl) {
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