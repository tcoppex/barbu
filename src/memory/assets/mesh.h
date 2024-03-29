#ifndef BARBU_MEMORY_ASSETS_MESH_H_
#define BARBU_MEMORY_ASSETS_MESH_H_

#include "memory/asset_factory.h"
#include "memory/resources/mesh_data.h"

#include "shaders/generic/interop.h"

// ----------------------------------------------------------------------------

using MeshParameters = AssetParameters;

// ----------------------------------------------------------------------------

class Mesh : public Asset<MeshParameters, MeshData> {
 public:
  enum AttributeBinding {
    ATTRIB_POSITION       = VERTEX_ATTRIB_POSITION,
    ATTRIB_TEXCOORD       = VERTEX_ATTRIB_TEXCOORD,
    ATTRIB_NORMAL         = VERTEX_ATTRIB_NORMAL,
    ATTRIB_TANGENT        = VERTEX_ATTRIB_TANGENT, //
    ATTRIB_JOINT_INDICES  = VERTEX_ATTRIB_JOINT_INDICES,
    ATTRIB_JOINT_WEIGHTS  = VERTEX_ATTRIB_JOINT_WEIGHTS,
    kNumVertexAttributeBinding
  };

 public:
  Mesh(Parameters_t const& _params)
    : Asset(_params)
  {}
  
  ~Mesh() {
    release();
  }

  inline bool loaded() const noexcept final {
    return vao_ != 0u;
  }

  // Draw the mesh using its internal mode when kInternal is set or the one provided otherwise.
  void draw(int32_t count = 1, MeshData::PrimitiveType primitive = MeshData::kInternal) const;
  void drawSubMesh(int32_t index, int32_t count = 1, MeshData::PrimitiveType primitive = MeshData::kInternal) const;

  inline int32_t nfaces() const noexcept { return nfaces_; }
  inline int32_t nvertices() const noexcept { return nvertices_; }

  // Return the number of sub geometry contains in the mesh, which is always at least 1.
  inline int32_t numSubMesh() const noexcept { return std::max( 1, static_cast<int32_t>(vgroups_.size())); };

  inline bool hasMaterials() const noexcept { return !vgroups_.empty(); }
  inline VertexGroups_t const& vertexGroups() const noexcept { return vgroups_; }
  inline VertexGroup const& vertexGroup(int32_t index) const { return vgroups_.at(index); }

  inline SkeletonHandle skeleton() { return skeleton_; }

  inline glm::vec3 const& centroid() const noexcept { return centroid_; }
  inline glm::vec3 const& bounds() const noexcept { return bounds_; }
  inline float radius() const noexcept { return radius_; }

 private:
  void allocate() final;
  void release() final;
  bool setup() final;

  uint32_t getInternalDrawMode(MeshData::PrimitiveType primitive) const; //

  // Buffer ids.
  uint32_t vao_ = 0u;
  uint32_t vbo_ = 0u;
  uint32_t skin_vbo_ = 0u;
  uint32_t ibo_ = 0u;

  // Drawing parameters.
  MeshData::PrimitiveType type_;
  int32_t nelems_    = 0;
  int32_t nvertices_ = 0;
  int32_t nfaces_    = 0;

  // Material specific submeshes.
  VertexGroups_t vgroups_;

  // Associated skeleton [tmp ?].
  SkeletonHandle skeleton_ = nullptr; //

  // Axis-Aligned Bounding box.
  glm::vec3 centroid_;
  glm::vec3 bounds_;
  float radius_;

 private:
  template<typename> friend class AssetFactory;
};

// ----------------------------------------------------------------------------

using MeshHandle = AssetHandle<Mesh>;

// ----------------------------------------------------------------------------

class MeshFactory : public AssetFactory<Mesh> {
 public:
  ~MeshFactory() { release_all(); }

  // Will try to create a resource and mesh asset from 'meshdata' with id
  // 'basename (#)' where ' (#)' is a numbered suffixed when the id is already taken.
  // (Note that the resource and asset could hence have different id).
  Handle add_object(std::string const& basename, MeshData const& meshdata);

  // Prebuilt meshes to create.
  Handle createPlane(float size = MeshData::kDefaultSize);
  Handle createGrid(int resolution, float size = MeshData::kDefaultSize);
  Handle createCube(float size = MeshData::kDefaultSize);
  Handle createWireCube(float size = MeshData::kDefaultSize);
  Handle createSphere(int xres = MeshData::kSphereDefaultXResolution, int yres = MeshData::kSphereDefaultYResolution, float radius = MeshData::kDefaultSize);
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_MESH_H_
