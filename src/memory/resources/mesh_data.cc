#include "memory/resources/mesh_data.h"

#include "glm/gtc/type_ptr.hpp"

#include "memory/assets/assets.h"

#define MATHUTILS_MAP_VECTOR
#include "utils/mathutils.h"

// ----------------------------------------------------------------------------

// Note : 
// The specialized mesh generators could be simplified by using the MeshData 
// structure directly but were kept for simplicity / legacy.

void MeshData::Plane(MeshData &mesh, float size) {
  RawMeshData raw;

  float const c{ 0.5f * size };
  raw.vertices = {
    glm::vec3{ -c, 0.0f, +c },   
    glm::vec3{ -c, 0.0f, -c },   
    glm::vec3{ +c, 0.0f, +c },   
    glm::vec3{ +c, 0.0f, -c },
  };

  raw.texcoords = {
    glm::vec2{ 0.0f, 1.0f }, 
    glm::vec2{ 0.0f, 0.0f },  
    glm::vec2{ 1.0f, 1.0f },  
    glm::vec2{ 1.0f, 0.0f },
  };

  // [ optional ]
  // for (int i = 0; i < 4; ++i) {
  //   raw.addIndex(i);
  // }

  mesh.setup( PrimitiveType::TRIANGLE_STRIP, raw );
}

// ----------------------------------------------------------------------------

void MeshData::Grid(MeshData &mesh, int resolution, float size) {
  int const nvertices   = 4 * (resolution + 1);
  int const ncomponents = 2;
  int const buffersize  = nvertices * ncomponents;

  float const cell_padding = size / static_cast<float>(resolution);
  float const offset       = size / 2.0f; //cell_padding * (res/2.0f);

  // Vertices datas.
  std::vector<float> lines(buffersize);
  for (int i = 0; i <= resolution; ++i) {

    // hack to have middle lines drawn last.
    int const i_offset{ (i < resolution/2) ? 0 : (i < resolution) ? 1 : -resolution/2 };

    // Position on the grid.
    float const fp_i{ static_cast<float>(i + i_offset) };
    float const cursor{ cell_padding * fp_i - offset };
    
    // buffer index.
    uint32_t const index{ 4u * ncomponents * i };

    // horizontal lines
    lines[index + 0u] = - offset;
    lines[index + 1u] = cursor;
    lines[index + 2u] = + offset;
    lines[index + 3u] = cursor;
    // vertical lines
    lines[index + 4u] = cursor;
    lines[index + 5u] = - offset;
    lines[index + 6u] = cursor;
    lines[index + 7u] = + offset; 
  }

  // Indices (as lines).
  RawMeshData raw;
  raw.vertices.resize(nvertices);
  for (int i = 0; i < nvertices; ++i) { 
    auto& dst = raw.vertices[i];
    dst[0] = lines[i*ncomponents + 0];
    dst[1] = lines[i*ncomponents + 1];
    dst[2] = 0.0;
  }

  mesh.setup( PrimitiveType::LINES, raw );
}

// ----------------------------------------------------------------------------

void MeshData::Cube(MeshData &mesh, float size) {
  constexpr int nfaces{ 6 };
  constexpr int nvertices{ 4 * nfaces };
  constexpr int nelems{ 6 * nfaces };

  float const c{ 0.5f * size };
  const std::array<float[3], nvertices> vertices{
    +c, +c, +c,   +c, -c, +c,   +c, -c, -c,   +c, +c, -c, // +X
    -c, +c, +c,   -c, +c, -c,   -c, -c, -c,   -c, -c, +c, // -X
    +c, +c, +c,   +c, +c, -c,   -c, +c, -c,   -c, +c, +c, // +Y
    +c, -c, +c,   -c, -c, +c,   -c, -c, -c,   +c, -c, -c, // -Y
    +c, +c, +c,   -c, +c, +c,   -c, -c, +c,   +c, -c, +c, // +Z
    +c, +c, -c,   +c, -c, -c,   -c, -c, -c,   -c, +c, -c  // -Z
  };

  constexpr std::array<float[2], nvertices> texcoords{
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
    1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f
  };

  constexpr std::array<float[3], nvertices> normals{
    +1.0f, 0.0f, 0.0f,  +1.0f, 0.0f, 0.0f,  +1.0f, 0.0f, 0.0f,  +1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,

    0.0f, +1.0f, 0.0f,  0.0f, +1.0f, 0.0f,  0.0f, +1.0f, 0.0f,  0.0f, +1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,

    0.0f, 0.0f, +1.0f,  0.0f, 0.0f, +1.0f,  0.0f, 0.0f, +1.0f,  0.0f, 0.0f, +1.0f,
    0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f
  };

  constexpr std::array<uint16_t, nelems> indices{
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7,
    8, 9, 10, 8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23
  };

  // -----

  RawMeshData raw;
  raw.vertices.resize(vertices.size());
  raw.texcoords.resize(texcoords.size());
  raw.normals.resize(normals.size());
  for (int i = 0; i < nvertices; ++i) { 
    // Positions  
    {
      auto const& src = vertices[i];
      auto& dst = raw.vertices[i];
      dst[0] = src[0];
      dst[1] = src[1];
      dst[2] = src[2];
    }
    // Texcoords
    {
      auto const& src = texcoords[i];
      auto& dst = raw.texcoords[i];
      dst[0] = src[0];
      dst[1] = src[1];
    }
    // Normals
    {
      auto const& src = normals[i];
      auto& dst = raw.normals[i];
      dst[0] = src[0];
      dst[1] = src[1];
      dst[2] = src[2];
    }
  }

  // Indices (as triangles).
  raw.elementsAttribs.reserve(indices.size());
  for (auto &index : indices) {
    raw.addIndex( index );
  }

  // -----

  mesh.setup( PrimitiveType::TRIANGLES, raw);
}

// ----------------------------------------------------------------------------

void MeshData::WireCube(MeshData &mesh, float size) {
  int constexpr ncomponents = 3;
  int constexpr nvertices   = 8;
  int constexpr nelems      = 24;
  int constexpr buffersize  = ncomponents * nvertices;
  
  float const c = 0.5f * size;

  std::array<float, buffersize> const vertices{
    +c, +c, +c,   +c, -c, +c,   +c, -c, -c,   +c, +c, -c,
    -c, +c, +c,   -c, -c, +c,   -c, -c, -c,   -c, +c, -c
  };

  std::array<uint8_t, nelems> constexpr indices{
    0, 1, 1, 2, 2, 3, 3, 0,
    4, 5, 5, 6, 6, 7, 7, 4,
    0, 4, 1, 5, 2, 6, 3, 7
  };

  // -----

  RawMeshData raw;
  raw.vertices.resize(nvertices);
  for (int i = 0; i < nvertices; ++i) { 
    auto& dst = raw.vertices[i];
    dst[0] = vertices[ncomponents*i + 0];
    dst[1] = vertices[ncomponents*i + 1];
    dst[2] = vertices[ncomponents*i + 2];
  }

  // Indices (as lines).
  raw.elementsAttribs.reserve(indices.size());
  for (auto &index : indices) {
    raw.addIndex( index );
  }

  // -----

  mesh.setup( PrimitiveType::LINES, raw);
}

// ----------------------------------------------------------------------------

void MeshData::Sphere(MeshData &mesh, int xres, int yres, float radius) {
  float constexpr Pi     = glm::pi<float>();
  float constexpr TwoPi  = 2.0f * Pi;

  RawMeshData raw;
  auto &Positions = raw.vertices;
  auto &Texcoords = raw.texcoords;
  auto &Normals   = raw.normals;
  // auto &Indices   = raw.elementsAttribs;

  int const cols = xres + 1;
  int const rows = yres + 1;

  // Vertices data.
  {
    float const DeltaX = 1.0f / static_cast<float>(xres);
    float const DeltaY = 1.0f / static_cast<float>(yres);

    int const nvertices = 2 + (rows-2) * cols;
    Positions.resize(nvertices);
    Texcoords.resize(nvertices);
    Normals.resize(nvertices);

    // first vertex.
    int vertex_id = 0;
    Normals[vertex_id]   = glm::vec3( 0.0f, -1.0f, 0.0f);
    Texcoords[vertex_id] = glm::vec2( 0.0f, 0.0f);
    Positions[vertex_id] = radius * Normals[vertex_id];
    ++vertex_id;

    for (int j = 1; j < rows-1; ++j) {
      float const dj = static_cast<float>(j) * DeltaY;

      float const theta = (dj - 0.5f) * Pi;
      float const ct = cosf(theta);
      float const st = sinf(theta);

      for (int i = 0; i < cols; ++i) {
        float const di = static_cast<float>(i) * DeltaX;

        float const phi = di * TwoPi;
        float const cp = cosf(phi);
        float const sp = sinf(phi);

        Normals[vertex_id]   = glm::normalize(glm::vec3( ct * cp, st, ct * sp));
        Texcoords[vertex_id] = glm::vec2( di, dj);
        Positions[vertex_id] = radius * Normals[vertex_id];
        ++vertex_id;
      }
    }

    // last vertex.
    Normals[vertex_id]   = glm::vec3( 0.0f, 1.0f, 0.0f);
    Texcoords[vertex_id] = glm::vec2( 1.0f, 1.0f);
    Positions[vertex_id] = radius * Normals[vertex_id];
    ++vertex_id;
  }

  // Indices.
  {
    size_t const nelems = 2 * cols * (rows-1) + 2 * (rows-3);
    raw.elementsAttribs.reserve(nelems);

    for (int32_t i = 0; i < cols; ++i) {
      raw.addIndex( 0 );
      raw.addIndex( 1 + i );
    }
    for (int32_t j = 1; j < rows-2; ++j) {
      int32_t const last_elem = raw.elementsAttribs.back().x;

      raw.addIndex( last_elem );
      raw.addIndex( last_elem );

      int32_t const first_vertex_id = last_elem - cols + 1;
      for (int32_t i = 0; i < cols; ++i) {
        raw.addIndex( first_vertex_id + i );
        raw.addIndex( first_vertex_id + i + cols );
      }
    }

    auto const npositions = static_cast<int32_t>(Positions.size());
    for (int32_t i = 0; i < cols; ++i) {
      raw.addIndex( npositions - cols - 1 + i );
      raw.addIndex( npositions - 1 );
    }
  }

  mesh.setup( PrimitiveType::TRIANGLE_STRIP, raw);
}

// ----------------------------------------------------------------------------

void MeshData::release() {
  VertexBuffer_t().swap( vertices );
  IndexBuffer_t().swap( indices );
}

// ----------------------------------------------------------------------------

bool MeshData::setup(PrimitiveType _type, RawMeshData &_raw, bool bNeedTangents) {
  auto const meshname = _raw.name.empty() ? "[mesh]" : Logger::TrimFilename(_raw.name);

  type = _type;
  std::swap(vgroups, _raw.vgroups);
   
  if (type != MeshData::TRIANGLES)
  {
    // When MeshData is not made of triangles we just copy the structures.

    // Elements.
    indices.reserve(_raw.elementsAttribs.size());
    for (auto const& vi : _raw.elementsAttribs) {
      indices.push_back(vi.x);
    }
    
    // Vertices.
    int const nvertices = static_cast<int>(_raw.vertices.size());
    vertices.resize( nvertices );
    for (int i = 0; i < nvertices; ++i) {
      vertices[i].position = _raw.vertices[i];
    }
    if (!_raw.texcoords.empty()) {
      for (int i = 0; i < nvertices; ++i) {
        vertices[i].texcoord = _raw.texcoords[i];
      }
    }
    if (!_raw.normals.empty()) {
      for (int i = 0; i < nvertices; ++i) {
        vertices[i].normal = _raw.normals[i];
      }
    }

    if (!_raw.tangents.empty()) {
      for (int i = 0; i < nvertices; ++i) {
        vertices[i].tangent = _raw.tangents[i]; // TODO ?
      }
    }
  }
  else 
  {
    bNeedTangents = bNeedTangents && _raw.tangents.empty();

    // Recalculate normals / tangents when none exists.
    if (!_raw.elementsAttribs.empty()) { 
      if (_raw.normals.empty()) {
        LOG_DEBUG_INFO( "Recalculating normals for :", meshname );
        _raw.recalculateNormals();
      }

      // [only recalculate tangent when the material contains normal map ?]
      if (bNeedTangents) {
        LOG_DEBUG_INFO( "Recalculating tangents for :", meshname );
        _raw.recalculateTangents();
      }
    }
    
    bool const has_tangent = !_raw.tangents.empty();

    // Reindexing vertices from sparse input.
    std::vector<glm::ivec4> attribIndices;
    attribIndices.reserve(_raw.vertices.size());

    indices.reserve(_raw.elementsAttribs.size());
    {
      MapVec3<glm::ivec3, size_t> mapVertices;

      auto const nelems = static_cast<int32_t>(_raw.elementsAttribs.size());
      for (int i = 0; i < nelems; ++i) {
        auto const& key = _raw.elementsAttribs[i];
        auto const& it = mapVertices.find(key);  // 

        int index = static_cast<int>(attribIndices.size());
        if (it == mapVertices.cend()) {
          mapVertices[key] = index;
          attribIndices.push_back( glm::ivec4(key, i) );
        } else {
          index = static_cast<int32_t>(it->second);  
        }
        
        indices.push_back(index);
      }
    }

    // Create an unique interleaved attribute vertices buffer.
    int const nvertices = static_cast<int>(attribIndices.size());
    
    vertices.resize(nvertices);
    for (int i = 0; i < nvertices; ++i) {
      auto const& index = attribIndices[i];

      // (positions)
      vertices[i].position = _raw.vertices[index.x];
      // (texcoords)
      if (index.y >= 0) {
        vertices[i].texcoord = _raw.texcoords[index.y];
      }
      // (normals)
      if (index.z >= 0) {
        vertices[i].normal = _raw.normals[index.z];
      }
      // (tangents)
      if (has_tangent) {
        vertices[i].tangent = _raw.tangents[ bNeedTangents ? index.w : i ]; // XXX XXX XXX
      }
    }
  
    // (skinning)
    if (!_raw.joints.empty()) {
      skinnings.resize(nvertices);
      for (int i = 0; i < nvertices; ++i) {
        auto const& index = attribIndices[i];
        skinnings[i].joint_indices = _raw.joints[index.x];
        skinnings[i].joint_weights = _raw.weights[index.x];
      }
    }
  }

  return true;
}

bool MeshData::setup(RawMeshFile &meshfile, bool bNeedTangents) {
  // Note : for now we process only a single mesh, but in the future we shall
  //        handle all sub objects from a file.

  auto &raw = meshfile.meshes[0]; //
  MeshData::PrimitiveType primtype = raw.elementsAttribs.empty() ? MeshData::POINTS
                                                                 : MeshData::TRIANGLES
                                                                 ;
  return setup( primtype, raw, bNeedTangents);
}

// ----------------------------------------------------------------------------

void MeshData::calculateBounds(glm::vec3 &pivot, glm::vec3 &bounds, float &radius) const {
  auto const resultX = std::minmax_element(vertices.begin(), vertices.end(), 
    [](auto const &a, auto const &b) { return a.position.x < b.position.x; }
  );
  auto const resultY = std::minmax_element(vertices.begin(), vertices.end(), 
    [](auto const &a, auto const &b) { return a.position.y < b.position.y; }
  );
  auto const resultZ = std::minmax_element(vertices.begin(), vertices.end(), 
    [](auto const &a, auto const &b) { return a.position.z < b.position.z; }
  );

  glm::vec3 const minBound(
    resultX.first->position.x, resultY.first->position.y, resultZ.first->position.z
  );
  glm::vec3 const maxBound(
    resultX.second->position.x, resultY.second->position.y, resultZ.second->position.z
  );

  pivot  = (maxBound + minBound) / 2.0f;
  bounds = glm::max(glm::abs(maxBound), glm::abs(minBound));
  radius = glm::max(glm::max(bounds.x, bounds.y), bounds.z);
}

// ----------------------------------------------------------------------------