#ifndef BARBU_MEMORY_RESOURCES_MESHDATA_H_
#define BARBU_MEMORY_RESOURCES_MESHDATA_H_

#include <string>
#include <vector>

#include "memory/resource_manager.h"

#include "utils/raw_mesh_file.h" // VertexGroups_t, RawMeshFile, MaterialFile

// ----------------------------------------------------------------------------
//
// Notes : 
//
//    * When materials are loaded, they are prefixed with the trimmed name of their file
//    along with their corresponding mesh's vertexgroup to avoid collision in
//    materials identifications.
//
//    * RawMeshFile are used to load sparse meshes data before postprocessing. 
//   Originally used to load Wavefront OBJ it could become overkill.
//
//    * One solution is to have different setup depending on input data.
//
//    * GLTF to Mesh data move 4x time (from hard drive to GPU), where they
//   could move only twice by keeping a pointer to a cgltf_data structure
//   inside MeshData.
//    HDD -> cgltf -> RawMeshFile -> MeshData -> Mesh
//      to
//    HDD -> (MeshData / cgltf) -> Mesh
//
//    * But this lake genericity for CPU use, so one solution is to have
//      different setup depending on input data to avoid useless convertion.
//
//    * MeshData could also completely change to handle scene data instead,
//   and thus potentially use cgltf_data directly. 
//   This could allowed easier cross application live update.
//
//    * Hence for now MeshDataManager load individual scene as a single joint object.
//   In the future scene as separated objects could be load either on SceneHierarchy
//   or here, if we change MeshData to SceneData (for example).
//
// ----------------------------------------------------------------------------


// [rename (eg. SceneData) ?]
// Interleaved vertex attributes (AoS-layout) on the host used to represent
// various mesh data : pincipally vertices mesh, but possibly materials, skeleton, etc..).
struct MeshData : public Resource {
  // Compatible primitive type.
  enum PrimitiveType {
    POINTS,
    LINES,
    TRIANGLES,
    TRIANGLE_STRIP,
    kNumPrimitiveType,
    kInternal
  };

  // Interleaved vertex structure.
  struct Vertex_t {
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
  };

  using VertexBuffer_t = std::vector<Vertex_t>;
  using IndexBuffer_t  = std::vector<uint32_t>;

  // Default vertex group identifier when there is none.
  static constexpr char const* kDefaultGroupName{ 
    "[Default]" 
  };

  // -------------------

  PrimitiveType  type;
  VertexBuffer_t vertices;
  IndexBuffer_t  indices;

  // Range of vertex indices representing sub-part of the mesh, used for materials.
  VertexGroups_t vgroups;

  // Material data associated to the mesh.
  MaterialFile material; //

  // -------------------

  /* Setup from a RawMeshData of specified primitive type. */
  bool setup(PrimitiveType _type, RawMeshData &_raw); //
  bool setup(RawMeshFile &meshfile);

  void release() final;

  inline bool loaded() const noexcept final { 
    return !vertices.empty();
  }

  /* Calculate the pivot and bound for the current vertices data. */
  void calculate_bounds(glm::vec3 &pivot, glm::vec3 &bounds) const;

  inline int32_t nvertices() const { 
    return static_cast<int32_t>(vertices.size()); 
  }
  
  inline int32_t nfaces() const { 
    return static_cast<int32_t>(indices.size() / 3); 
  }

  inline bool has_materials() const { 
    return !material.infos.empty(); 
  }

  // -- Canonical Meshes ---
  static constexpr float kGridDefaultSize         = 1.0f;
  static constexpr float kCubeDefaultSize         = 1.0f;
  static constexpr float kSphereDefaultRadius     = 1.0f;
  static constexpr int kSphereDefaultXResolution  = 32;
  static constexpr int kSphereDefaultYResolution  = kSphereDefaultXResolution;

  static void Grid(MeshData &mesh, int resolution, float size = kGridDefaultSize);
  static void Cube(MeshData &mesh, float size = kCubeDefaultSize);
  static void WireCube(MeshData &mesh, float size = kCubeDefaultSize);
  static void Sphere(MeshData &mesh, int xres = kSphereDefaultXResolution, int yres = kSphereDefaultYResolution, float radius = kSphereDefaultRadius);
};

// ----------------------------------------------------------------------------

class MeshDataManager : public ResourceManager<MeshData> {
 public:
  static bool CheckExtension(std::string_view ext);

 private:
  Handle _load(ResourceId const& id) final;
  Handle _load_internal(ResourceId const& id, int32_t size, void const* data, std::string_view mime_type) final { return Handle(); }

  // Load file as single mesh.
  bool load_obj(std::string_view filename, MeshData &mesh);
  bool load_gltf(std::string_view filename, MeshData &mesh);
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCES_MESHDATA_H_
