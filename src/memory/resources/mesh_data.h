#ifndef BARBU_MEMORY_RESOURCES_MESHDATA_H_
#define BARBU_MEMORY_RESOURCES_MESHDATA_H_

#include <string>
#include <vector>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "memory/resources/resource_manager.h"

// ----------------------------------------------------------------------------

// Vertex Group.
// Defines a subpart of a mesh, represented by a range of vertex indices.
struct VertexGroup {
  int32_t nelems() const { 
    return end_index - start_index;
  }

  std::string name;
  int32_t start_index = -1;
  int32_t end_index = -1;
};

using VertexGroups_t = std::vector<VertexGroup>;

// ----------------------------------------------------------------------------

// RawMesh.
// Structure of sparse attributes arrays (SoA-layout) used to built a MeshData
// or import one from an external source.
struct RawMeshData {
  static constexpr size_t kDefaultTriangleCapacity = 2048;
  static constexpr size_t kDefaultCapacity = 3 * kDefaultTriangleCapacity;

  RawMeshData(size_t capacity = kDefaultCapacity) {
    vertices.reserve(capacity);
    texcoords.reserve(capacity);
    normals.reserve(capacity);
    elementsAttribs.reserve(capacity);
  }

  // Use elementsAttribs & vertices to fill the normals attributes.
  void recalculate_normals();

  inline int32_t nvertices() const { 
    return static_cast<int32_t>(vertices.size());
  }
  
  inline int32_t nfaces() const { 
    return elementsAttribs.empty() ? 0 : static_cast<int32_t>(elementsAttribs.size()/3); 
  }

  bool has_vertex_groups() const { 
    return !vgroups.empty();
  }

  // Mesh name, could be empty.
  std::string name;

  // List of unique vertex attributes (they do not necesserally have the same size).
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> texcoords;
  std::vector<glm::vec3> normals;

  // List of triangle faces, each elements is a vertex containing indices to its attribute.
  // (respectively position, texcoord, normals). Contains 3 * nfaces elements.
  std::vector<glm::ivec3> elementsAttribs;

  // Sub-geometry description.
  VertexGroups_t vgroups;
};

// ----------------------------------------------------------------------------

// Material Info.
struct MaterialInfo {
  std::string name;

  glm::vec4 diffuse_color{1.0f};      // RGB Albedo + Alpha
  glm::vec4 specular_color{0.0f};     // XYZ specular + W Exponent

  std::string diffuse_map;
  std::string specular_map;
  std::string bump_map;
  std::string alpha_map;
};

// MaterialFile.
// Defines a set of material objects info.
struct MaterialFile {
  std::string id;
  std::vector<MaterialInfo> infos;

  inline MaterialInfo const& get(size_t index) const {
    return infos.at(index);
  }
};

// ----------------------------------------------------------------------------

// RawMeshFile.
// Represents a list of mesh objects within a single file possibly
// sharing a common material file.
struct RawMeshFile {
  static constexpr size_t kDefaultCapacity = 4;

  RawMeshFile(size_t capacity = kDefaultCapacity) {
    meshes.reserve(capacity);
  }

  std::string material_id;            // Material file id / relative path name.
  std::vector<RawMeshData> meshes;    // Buffer of meshes.
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// [rename (eg. SceneData)]
// Interleaved vertex attributes (AoS-layout) on the host used to represent
// various mesh data : pincipally vertices mesh, but possibly materials, skeleton, etc..).
struct MeshData : public Resource {
 public:
  enum PrimitiveType {
    POINTS,
    LINES,
    TRIANGLES,
    TRIANGLE_STRIP,
    kNumPrimitiveType,
    kInternal
  };

  struct Vertex_t {
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
  };

  using VertexBuffer_t = std::vector<Vertex_t>;
  using IndexBuffer_t  = std::vector<uint32_t>;

  static constexpr char const* kDefaultGroupName{ "[Default]" };
  
 public:
  static constexpr float kGridDefaultSize         = 1.0f;
  static constexpr float kCubeDefaultSize         = 1.0f;
  static constexpr float kSphereDefaultRadius     = 1.0f;
  static constexpr int kSphereDefaultXResolution  = 32;
  static constexpr int kSphereDefaultYResolution  = kSphereDefaultXResolution;

  static void Grid(MeshData &mesh, int resolution, float size = kGridDefaultSize);
  static void Cube(MeshData &mesh, float size = kCubeDefaultSize);
  static void WireCube(MeshData &mesh, float size = kCubeDefaultSize);
  static void Sphere(MeshData &mesh, int xres = kSphereDefaultXResolution, int yres = kSphereDefaultYResolution, float radius = kSphereDefaultRadius);

 public:
  void release() final;

  inline bool loaded() const noexcept final { return !vertices.empty(); }

  // Setup the internal vertices & indices data from a RawMeshData of specified primitive type.
  void setup(PrimitiveType type, RawMeshData &raw); //

  // Calculate the pivot and bound for the current vertices data.
  void calculate_bounds(glm::vec3 &pivot, glm::vec3 &bounds) const;

  inline int32_t nvertices() const { return static_cast<int32_t>(vertices.size()); }
  inline int32_t nfaces() const { return static_cast<int32_t>(indices.size() / 3); }

  inline bool has_materials() const { return !material.infos.empty(); }

 public:
  PrimitiveType  type = PrimitiveType::kInternal;
  VertexBuffer_t vertices;
  IndexBuffer_t  indices;

  // Range of vertex indices representing sub-part of the mesh, used for materials.
  VertexGroups_t vgroups;

  // Material data associated ot the mesh.
  MaterialFile material; //
};

// ----------------------------------------------------------------------------

class MeshDataManager : public ResourceManager<MeshData> {
 private:
  Handle _load(ResourceId const& id) final;

  // Load a Wavefront obj into the specified mesh. Return true when it succeeds.
  bool load_obj(std::string_view filename, MeshData &mesh);
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCES_MESHDATA_H_
