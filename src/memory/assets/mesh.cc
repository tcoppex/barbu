#include "memory/assets/mesh.h"
#include "core/graphics.h"

// ----------------------------------------------------------------------------

void Mesh::draw(MeshData::PrimitiveType primitive) const {
  for (auto i = 0; i < nsubgeometry(); ++i) {
    draw_submesh(i, primitive);
  }
}

void Mesh::draw_submesh(int32_t index, MeshData::PrimitiveType primitive) const {
  assert( loaded() );

  auto const mode = get_draw_mode(primitive);

  glBindVertexArray(vao_);
  if (!vgroups_.empty()) {
    auto const& vg = vgroups_.at(index);
    glDrawElements( mode, vg.nelems(), GL_UNSIGNED_INT, reinterpret_cast<void*>(vg.start_index*sizeof(uint32_t)));
    
    //glDrawRangeElements(mode, vg.start_index, vg.end_index, vg.nelems() * sizeof(uint32_t), GL_UNSIGNED_INT, nullptr); // XXX    
    //glDrawArrays( mode, vg.start_index, vg.nelems());

  } else if (nelems_ <= 0) {
    glDrawArrays(mode, 0u, nvertices_);
  } else if (index <= 0) {
    glDrawElements(mode, nelems_, GL_UNSIGNED_INT, nullptr);  
  } else {
    LOG_WARNING( __FUNCTION__, ": invalid parameters." );
  }

  glBindVertexArray(0u);
}

// ----------------------------------------------------------------------------

uint32_t Mesh::get_draw_mode(MeshData::PrimitiveType primitive) const {
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
    glCreateBuffers(1u, &vbo_);
    glCreateBuffers(1u, &ibo_);
    glCreateVertexArrays(1u, &vao_);
  }
  CHECK_GX_ERROR();
}

void Mesh::release() {
  if (loaded()) {
    glDeleteBuffers(1u, &vbo_);
    glDeleteBuffers(1u, &ibo_);
    glDeleteVertexArrays(1u, &vao_);
    vbo_ = ibo_ = vao_ = 0u;
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
  vgroups_    = meshdata.vgroups;

  // Calculate the mesh AABB.
  meshdata.calculate_bounds( pivot_, bounds_);

  // Recenter the mesh to its pivot.
  // [todo ?]
  // at least horizontally / XZ plane, or alternatively suggest a default transform.

  // Reallocate buffers if the data are reuploads.
  if (resource.version > 0 && loaded()) {
    release();
    allocate();
  }
  
  // Setup internal buffers.
  uint32_t bytesize = static_cast<uint32_t>(nvertices_ * sizeof(meshdata.vertices[0u]));
  glNamedBufferStorage(vbo_, bytesize, meshdata.vertices.data(), 0);
  
  glBindVertexArray(vao_);
  {
    auto constexpr binding_index = 0u;
    glBindVertexBuffer(binding_index, vbo_, 0u, sizeof(MeshData::Vertex_t));
    {
      uint32_t attrib_index, num_component;

      attrib_index = ATTRIB_POSITION;
      num_component = static_cast<uint32_t>((sizeof MeshData::Vertex_t::position) / sizeof(MeshData::Vertex_t::position[0u]));
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, offsetof(MeshData::Vertex_t, position));
      glVertexAttribBinding(attrib_index, binding_index);
      glEnableVertexAttribArray(attrib_index);

      attrib_index = ATTRIB_TEXCOORD;
      num_component = static_cast<uint32_t>((sizeof MeshData::Vertex_t::texcoord) / sizeof(MeshData::Vertex_t::texcoord[0u]));
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, offsetof(MeshData::Vertex_t, texcoord));
      glVertexAttribBinding(attrib_index, binding_index);
      glEnableVertexAttribArray(attrib_index);

      attrib_index = ATTRIB_NORMAL;
      num_component = static_cast<uint32_t>((sizeof MeshData::Vertex_t::normal) / sizeof(MeshData::Vertex_t::normal[0u]));
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, offsetof(MeshData::Vertex_t, normal));
      glVertexAttribBinding(attrib_index, binding_index);
      glEnableVertexAttribArray(attrib_index);
    }
  }
  glBindVertexArray(0u);

  // Faces indices.
  if (!meshdata.indices.empty()) {
    bytesize = static_cast<uint32_t>(nelems_ * sizeof(meshdata.indices[0u]));
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
  // they do not collide any others.
  auto const id = AssetId::FindUnique( basename, 
    [this](AssetId const& _id) {
      return has(_id);
    }
  );

  Parameters_t params;
  params.dependencies.add_resource( Resources::Add<MeshData>( basename, meshdata) );
  
  return create( id, params );
}

MeshFactory::Handle MeshFactory::createGrid(int resolution, float size) {
  MeshData meshdata;
  MeshData::Grid( meshdata, resolution, size);
  return add_object( "grid", meshdata );
}

MeshFactory::Handle MeshFactory::createCube(float size) {
  MeshData meshdata;
  MeshData::Cube( meshdata, size);
  return add_object( "cube", meshdata );
}

MeshFactory::Handle MeshFactory::createWireCube(float size) {
  MeshData meshdata;
  MeshData::WireCube( meshdata, size);
  return add_object( "wirecube", meshdata );
}

MeshFactory::Handle MeshFactory::createSphere(int xres, int yres, float radius) {
  MeshData meshdata;
  MeshData::Sphere( meshdata, xres, yres, radius);
  return add_object( "sphere", meshdata );
}

// ----------------------------------------------------------------------------
