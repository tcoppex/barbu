#include "memory/assets/mesh.h"
#include "core/graphics.h"

// ----------------------------------------------------------------------------

void Mesh::draw(int32_t count, MeshData::PrimitiveType primitive) const {
  for (auto i = 0; i < numSubMesh(); ++i) {
    drawSubMesh(i, count, primitive);
  }
}

void Mesh::drawSubMesh(int32_t index, int32_t count, MeshData::PrimitiveType primitive) const {
  assert( loaded() );

  auto const mode = getInternalDrawMode(primitive);

  glBindVertexArray(vao_);
  if (nelems_ > 0) {
    int32_t nelems{ nelems_ };
    void* offset{ nullptr };

    if (!vgroups_.empty()) {
      auto const& vg = vgroups_.at(index);
      nelems = vg.nelems();
      offset = reinterpret_cast<void*>(vg.start_index * sizeof(uint32_t));
    }
    glDrawElementsInstanced( mode, nelems, GL_UNSIGNED_INT, offset, count);
  } else {
    glDrawArraysInstanced(mode, 0u, nvertices_, count);
  }
  glBindVertexArray(0u);

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

uint32_t Mesh::getInternalDrawMode(MeshData::PrimitiveType primitive) const {
  GLenum mode;

  // When primitive is set to kInternal, use the mesh internal type.
  primitive = (primitive == MeshData::kInternal) ? type_ : primitive;

  // Translate to API-specific mode.
  switch (primitive) {
    case MeshData::LINES:
      mode = GL_LINES;
    break;
    
    case MeshData::TRIANGLES:
      mode = GL_TRIANGLES;
    break;

    case MeshData::TRIANGLE_STRIP:
      mode = GL_TRIANGLE_STRIP;
    break;

    case MeshData::POINTS:
    default:
      mode = GL_POINTS;
    break; 
  };

  return mode;
}

void Mesh::allocate() {
  if (!loaded()) {
    glCreateVertexArrays(1u, &vao_);
    glCreateBuffers(1u, &vbo_);
    glCreateBuffers(1u, &skin_vbo_);
    glCreateBuffers(1u, &ibo_);
  }
  CHECK_GX_ERROR();
}

void Mesh::release() {
  if (loaded()) {
    glDeleteBuffers(1u, &vbo_);
    glDeleteBuffers(1u, &skin_vbo_);
    glDeleteBuffers(1u, &ibo_);
    glDeleteVertexArrays(1u, &vao_);
    vbo_ = skin_vbo_ = ibo_ = vao_ = 0u;
  }
  CHECK_GX_ERROR();
}

bool Mesh::setup() {
  assert( !params.dependencies.empty() );

  // Load MeshData resource.
  auto &resource = params.dependencies[0];
  auto handle = Resources::GetUpdated<Resource_t>( resource );
  if (!handle.is_valid()) {
    return false; 
  }
  auto &meshdata = *(handle.data);

  // Generic parameters transfer.
  type_       = meshdata.type;
  nelems_     = static_cast<int32_t>(meshdata.indices.size());
  nvertices_  = static_cast<int32_t>(meshdata.vertices.size());
  nfaces_     = meshdata.nfaces();
  vgroups_    = meshdata.vgroups;

  skeleton_ = meshdata.skeleton;

  // Calculate the mesh AABB.
  meshdata.calculateBounds( centroid_, bounds_, radius_);

  // [Recenter the mesh to its pivot ?]
  // at least horizontally / XZ plane, or alternatively suggest a default transform.

  // Reallocate buffers if the data are reuploads.
  if (resource.version > 0 && loaded()) {
    release();
    allocate();
  }

  // [use bindless vao instead ?]
  glBindVertexArray(vao_);
  {
    uint32_t binding_index = 0u;
    
    // 1) Generic Vertex Attribs.
    {
      uint32_t const bytesize{
        static_cast<uint32_t>(nvertices_ * (sizeof meshdata.vertices[0u]))
      };
      glNamedBufferStorage(vbo_, bytesize, meshdata.vertices.data(), 0);
      
      glBindVertexBuffer(binding_index, vbo_, 0u, sizeof(MeshData::Vertex_t));
      {
        #define MESH_SetupAttrib(tAttrib, tComponent) \
        { \
          auto const ncomp{ static_cast<uint32_t>((sizeof MeshData::Vertex_t::tComponent) / (sizeof MeshData::Vertex_t::tComponent[0u]))}; \
          glVertexAttribFormat( Mesh::tAttrib, ncomp, GL_FLOAT, GL_FALSE, offsetof(MeshData::Vertex_t, tComponent)); \
          glVertexAttribBinding( Mesh::tAttrib, binding_index); \
          glEnableVertexAttribArray( Mesh::tAttrib ); \
        }

        MESH_SetupAttrib( ATTRIB_POSITION,  position );
        MESH_SetupAttrib( ATTRIB_TEXCOORD,  texcoord );
        MESH_SetupAttrib( ATTRIB_NORMAL,    normal );
        MESH_SetupAttrib( ATTRIB_TANGENT,   tangent );

        #undef MESH_SetupAttrib
      }
      ++binding_index;
    }

    // 2) Skinning Attribs.
    if (!meshdata.skinnings.empty()) {
      uint32_t const bytesize{
        static_cast<uint32_t>(nvertices_ * (sizeof meshdata.skinnings[0u]))
      };
      glNamedBufferStorage(skin_vbo_, bytesize, meshdata.skinnings.data(), 0);

      glBindVertexBuffer(binding_index, skin_vbo_, 0, sizeof(MeshData::Skinning_t));
      {
        uint32_t attrib_index, num_component;

        attrib_index = Mesh::ATTRIB_JOINT_INDICES;
        num_component = static_cast<uint32_t>((sizeof MeshData::Skinning_t::joint_indices) / (sizeof MeshData::Skinning_t::joint_indices[0u]));
        glVertexAttribIFormat(attrib_index, num_component, GL_UNSIGNED_INT, offsetof(MeshData::Skinning_t, joint_indices));
        glVertexAttribBinding(attrib_index, binding_index);
        glEnableVertexAttribArray(attrib_index);

        attrib_index = Mesh::ATTRIB_JOINT_WEIGHTS;
        num_component = static_cast<uint32_t>((sizeof MeshData::Skinning_t::joint_weights) / (sizeof MeshData::Skinning_t::joint_weights[0u]));
        glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_TRUE/**/, offsetof(MeshData::Skinning_t, joint_weights));
        glVertexAttribBinding(attrib_index, binding_index);
        glEnableVertexAttribArray(attrib_index);
      }
      ++binding_index;
    }
  }
  glBindVertexArray(0u);

  // Faces indices.
  if (!meshdata.indices.empty()) {
    uint32_t const bytesize{
      static_cast<uint32_t>(nelems_ * (sizeof meshdata.indices[0u]))
    };
    glNamedBufferStorage(ibo_, bytesize, meshdata.indices.data(), 0);
    glVertexArrayElementBuffer(vao_, ibo_);
  }
  CHECK_GX_ERROR();

  return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

MeshFactory::Handle MeshFactory::add_object(std::string const& basename, MeshData const& meshdata) {
  // The Resource id and asset id could be different, but at least we are sure
  // they do not collide with any others mesh asset.
  auto const id = AssetId::FindUnique( basename, 
    [this](AssetId const& _id) {
      return has(_id);
    }
  );

  Parameters_t params;
  params.dependencies.add_resource( Resources::Add<MeshData>( basename, meshdata) );
  
  return create( id, params );
}

MeshFactory::Handle MeshFactory::createPlane(float size) {
  MeshData meshdata;
  MeshData::Plane( meshdata, size);
  return add_object( "PlaneMesh", meshdata);
}

MeshFactory::Handle MeshFactory::createGrid(int resolution, float size) {
  MeshData meshdata;
  MeshData::Grid( meshdata, resolution, size);
  return add_object( "GridMesh", meshdata );
}

MeshFactory::Handle MeshFactory::createCube(float size) {
  MeshData meshdata;
  MeshData::Cube( meshdata, size);
  return add_object( "CubeMesh", meshdata );
}

MeshFactory::Handle MeshFactory::createWireCube(float size) {
  MeshData meshdata;
  MeshData::WireCube( meshdata, size);
  return add_object( "WireCubeMesh", meshdata );
}

MeshFactory::Handle MeshFactory::createSphere(int xres, int yres, float radius) {
  MeshData meshdata;
  MeshData::Sphere( meshdata, xres, yres, radius);
  return add_object( "SphereMesh", meshdata );
}

// ----------------------------------------------------------------------------
