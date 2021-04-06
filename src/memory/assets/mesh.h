#ifndef BARBU_MEMORY_ASSETS_MESH_H_
#define BARBU_MEMORY_ASSETS_MESH_H_

#include "memory/assets/asset_factory.h"
#include "memory/resources/mesh_data.h"

// ----------------------------------------------------------------------------

using MeshParameters = AssetParameters;

// ----------------------------------------------------------------------------

class Mesh : public Asset<MeshParameters, MeshData> {
 public:
  enum AttributeBinding {
    ATTRIB_POSITION = 0,
    ATTRIB_TEXCOORD = 1,
    ATTRIB_NORMAL   = 2,
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
  void draw(MeshData::PrimitiveType primitive = MeshData::kInternal) const;
  void draw_submesh(int32_t index, MeshData::PrimitiveType primitive = MeshData::kInternal) const;

  inline int32_t nfaces() const noexcept { return nelems_ / 3; }
  inline int32_t nvertices() const noexcept { return nvertices_; }

  // Return the number of sub geometry contains in the mesh, which is always at least 1.
  inline int32_t nsubgeometry() const noexcept { return std::max( 1, static_cast<int32_t>(vgroups_.size())); };

  inline bool has_materials() const noexcept { return !vgroups_.empty(); }
  inline VertexGroups_t const& vertex_groups() const noexcept { return vgroups_; }
  inline VertexGroup const& vertex_group(int32_t index) const { return vgroups_.at(index); }

  inline glm::vec3 const& pivot() const noexcept { return pivot_; }
  inline glm::vec3 const& bounds() const noexcept { return bounds_; }

 private:
  void allocate() final;
  void release() final;
  bool setup() final;

  uint32_t get_draw_mode(MeshData::PrimitiveType primitive) const;

  // Buffer ids.
  uint32_t vao_ = 0u;
  uint32_t vbo_ = 0u;
  uint32_t ibo_ = 0u;

  // Drawing parameters.
  MeshData::PrimitiveType type_;
  int32_t nelems_    = 0;
  int32_t nvertices_ = 0;

  // Material specific submeshes.
  VertexGroups_t vgroups_;

  // Axis-Aligned Bounding box.
  glm::vec3 pivot_;
  glm::vec3 bounds_;

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
  Handle createGrid(int resolution, float size = MeshData::kGridDefaultSize);
  Handle createCube(float size = MeshData::kCubeDefaultSize);
  Handle createWireCube(float size = MeshData::kCubeDefaultSize);
  Handle createSphere(int xres = MeshData::kSphereDefaultXResolution, int yres = MeshData::kSphereDefaultYResolution, float radius = MeshData::kSphereDefaultRadius);
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_MESH_H_
