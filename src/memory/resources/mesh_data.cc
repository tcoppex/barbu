#include "memory/resources/mesh_data.h"

#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cmath>
#include <sstream>
#include <map>

#include "glm/glm.hpp"    // abs

// ----------------------------------------------------------------------------

// Vertex ordering operator used to reindex vertices from raw data.
namespace {

template<typename T>
constexpr bool almost_equal(T const& a, T const& b) {
  //static_assert( std::is_floating_point<T>::value );
  T const distance = glm::abs(b - a);
  return (distance <= std::numeric_limits<T>::epsilon())
      || (distance < (std::numeric_limits<T>::min() * glm::abs(b + a)));
}

// Order comparison between two vec3.
template<typename T>
struct VecOrdering_t {
  static
  bool Less(const T& a, const T& b) {
    return (a.x < b.x)
        || (almost_equal(a.x, b.x) && (a.y < b.y))
        || (almost_equal(a.x, b.x) && almost_equal(a.y, b.y) && (a.z < b.z));
  }

  bool operator() (const T& a, const T& b) const {
    return Less(a, b);
  }
};

} // namespace

// ----------------------------------------------------------------------------

void RawMeshData::recalculate_normals() {
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

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Note : 
// The specialized mesh generators could be simplified by using the MeshData 
// structure directly but were kept for simplicity, due to legacy.

void MeshData::Grid(MeshData &mesh, int resolution, float size) {
  int const nvertices   = 4 * (resolution + 1);
  int const ncomponents = 2;
  int const buffersize  = nvertices * ncomponents;

  float const cell_padding = size / static_cast<float>(resolution);
  float const offset       = size / 2.0f; //cell_padding * (res/2.0f);

  // Vertices datas.
  std::vector<float> lines(buffersize);
  for (int i = 0; i <= resolution; ++i) {
    uint32_t const index{ 4u * ncomponents * i };    
    float const cursor{ cell_padding * static_cast<float>(i) - offset };

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
  for (int i = 0; i < nvertices; ++i)
  { 
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
  for (int i = 0; i < nvertices; ++i)
  { 
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
  raw.elementsAttribs.resize(indices.size());
  for (int i = 0; i < nelems; ++i) {
    auto const& index = indices[i];
    auto& dst = raw.elementsAttribs[i];
    dst.x = dst.y = dst.z = index;
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
  raw.elementsAttribs.resize(indices.size());
  for (int i = 0; i < nelems; ++i) {
    auto const& index = indices[i];
    raw.elementsAttribs[i] = glm::ivec3( index );
  }

  // -----

  mesh.setup( PrimitiveType::LINES, raw);
}

// ----------------------------------------------------------------------------

void MeshData::Sphere(MeshData &mesh, int xres, int yres, float radius) {
  float constexpr Pi     = M_PI;
  float constexpr TwoPi  = 2.0f * Pi;

  RawMeshData raw;
  auto &Positions = raw.vertices;
  auto &Texcoords = raw.texcoords;
  auto &Normals   = raw.normals;
  auto &Indices   = raw.elementsAttribs;

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
      float const dj = j * DeltaY;

      float const theta = (dj - 0.5f) * Pi;
      float const ct = cosf(theta);
      float const st = sinf(theta);

      for (int i = 0; i < cols; ++i) {
        float const di = i * DeltaX;

        float const phi = di * TwoPi;
        float const cp = cosf(phi);
        float const sp = sinf(phi);

        Normals[vertex_id]   = glm::normalize(glm::vec3( ct * cp, st, ct * sp));
        Texcoords[vertex_id] = glm::vec2( i*DeltaX, j*DeltaY );
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

  // Indices datas (as tristrip).
  {
    int const nelems = 2 * cols * (rows-1) + 2 * (rows-3);
    Indices.resize(nelems);

    int index = 0;
    for (int i = 0; i < cols; ++i) {
      Indices[index++] = glm::ivec3( 0 );
      Indices[index++] = glm::ivec3( 1 + i );
    }
    for (int j = 1; j < rows-2; ++j) {
      Indices[index] = Indices[index-1]; ++index;
      Indices[index] = Indices[index-1]; ++index;

      int const first_vertex_id = Indices[index-1].x - cols + 1;
      for (int i = 0; i < cols; ++i) {
        Indices[index++] = glm::ivec3( first_vertex_id + i );
        Indices[index++] = glm::ivec3( first_vertex_id + i + cols );
      }
    }
    for (int i = 0; i < cols; ++i) {
      Indices[index++] = glm::ivec3( Positions.size() - cols - 1 + i );
      Indices[index++] = glm::ivec3( Positions.size() - 1 );
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

void MeshData::setup(PrimitiveType primtype, RawMeshData &raw) {
  type = primtype;
  std::swap(vgroups, raw.vgroups);
   
  if (type != MeshData::TRIANGLES)
  {
    // When MeshData is not made of triangles we just copy the structures.

    // Elements.
    indices.reserve(raw.elementsAttribs.size());
    for (auto const& vi : raw.elementsAttribs) {
      indices.push_back(vi.x);
    }
    
    // Vertices.
    int const nvertices = static_cast<int>(raw.vertices.size());
    vertices.resize( nvertices );
    for (int i = 0; i < nvertices; ++i) {
      vertices[i].position = raw.vertices[i];
    }
    if (!raw.texcoords.empty()) {
      for (int i = 0; i < nvertices; ++i) {
        vertices[i].texcoord = raw.texcoords[i];
      }
    }
    if (!raw.normals.empty()) {
      for (int i = 0; i < nvertices; ++i) {
        vertices[i].normal = raw.normals[i];
      }
    }
  }
  else 
  {
    // Recalculate normals when none exists.
    if (raw.normals.empty() && !raw.elementsAttribs.empty()) {
      raw.recalculate_normals();
    }

    // Reindexing vertices from sparse input.
    std::vector<glm::ivec3> attribIndices;
    attribIndices.reserve(raw.vertices.size());
    indices.reserve(raw.elementsAttribs.size());
    {
      std::map<glm::ivec3, size_t, VecOrdering_t<glm::ivec3>> mapVertices;
      
      for (auto const& elem : raw.elementsAttribs) {
        glm::vec3 key(elem.x, elem.y, elem.z);
        auto it = mapVertices.find(key);

        int index = static_cast<int>(attribIndices.size());
        if (it == mapVertices.end()) {
          mapVertices[key] = index;
          attribIndices.push_back(key);
        } else {
          index = it->second;
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
      vertices[i].position = raw.vertices[index.x];
      // (texcoords)
      if (index.y >= 0) {
        vertices[i].texcoord = raw.texcoords[index.y];
      }
      // (normals)
      if (index.z >= 0) {
        vertices[i].normal = raw.normals[index.z];
      }
    }
  }
}

// ----------------------------------------------------------------------------

void MeshData::calculate_bounds(glm::vec3 &pivot, glm::vec3 &bounds) const {
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
  bounds = maxBound; 
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

namespace {

// Load a file using an external buffer.
//
// If the buffer is empty or its size is not sufficient it will allocate it.
// Return true when the file has been loaded correctly, false otherwhise.
//
// When true is returned the user is responsible for the buffer deallocation,
// otherwhise it will be freed automatically.
bool LoadFile(std::string_view filename, char **buffer, size_t *buffersize) {
  assert( (buffer != nullptr) && (buffersize != nullptr) );
  FILE *fd = fopen( filename.data(), "r");
  
  if (nullptr == fd) {
    LOG_ERROR( filename, "does not exists.");
    return false;
  }

  // Determine its size.
  fseek(fd, 0, SEEK_END);
  size_t const filesize = static_cast<size_t>(ftell(fd));
  fseek(fd, 0, SEEK_SET);

  // Allocate the buffer.
  if ((*buffer != nullptr) && (*buffersize < filesize)) {
    delete [] *buffer;
    *buffer = nullptr;
  }
  if (*buffer == nullptr) {
    *buffer = new char[filesize]();
    *buffersize = filesize;
  }

  if (*buffer == nullptr) {
    LOG_ERROR( filename, " : buffer allocation failed.");
    fclose(fd);
    return false; 
  }

  // Read it.
  size_t const rbytes = fread(*buffer, 1u, filesize, fd);
  fclose(fd);

  bool const read_succeed = (rbytes == filesize);
  
  if (!read_succeed) {
    LOG_ERROR( "Fail to load file :", filename);
    delete [] *buffer;
    return false;
  }
  
  return read_succeed;
}

// This method parses all the geometry and fed 'raw' in one pass.
// It is destructive of the input buffer but should always works,
// note however that the data could be easily restored after each iteration.
void ParseOBJ(char *input, RawMeshFile &meshfile, bool bSeparateObjects) {
  using Vec2f = glm::vec2;
  using Vec3f = glm::vec3;
  using Vec3i = glm::ivec3;

  char namebuffer[128]{'\0'};

  char *s = input;
  for (char *end = strchr(s, '\n'); end != nullptr; s = end+1u, end = strchr(s, '\n'))
  {
    *end = '\0';

    // Handle Objects, Submesh & Material file.
    if (s[0] & 1) {
      // OBJECT
      if (s[0] == 'o') {

        // [wip]
        if (bSeparateObjects) {
          // [ Objects are defined by vertices, but maybe the whole OBJ is considered
          // as a unique buffer, so test it ] 
          meshfile.meshes.resize(meshfile.meshes.size() + 1);

          sscanf( s+2, "%128s", namebuffer);
          meshfile.meshes.back().name = std::string(namebuffer);
        }
      } 
      // MATERIAL ID (usemtl)
      else if (s[0] == 'u') {
        auto &raw = meshfile.meshes.back();

        int32_t const last_vertex_index = static_cast<int32_t>(raw.elementsAttribs.size()) - 1;

        // Set last index of current submesh.
        if (!raw.vgroups.empty()) {
          raw.vgroups.back().end_index = last_vertex_index;
        }

        // Add a new submesh.
        raw.vgroups.resize(raw.vgroups.size() + 1);
        auto &vg = raw.vgroups.back();

        // Set name and first vertex index.
        sscanf( s+7, "%128s", namebuffer);
        vg.name = std::string(namebuffer);
        vg.start_index = last_vertex_index + 1;
      } 
      // MATERIAL FILE ID (mtlib)
      else if ((s[0] == 'm') && (s[1] == 't')) {
        sscanf( s+7, "%128s", namebuffer);
        meshfile.material_id = std::string(namebuffer);
      }

      // [the other captured parameters has their first char last bit set to 0, so we can skip here]
      continue;
    }

    // If no objects are specified, create a default raw geo.
    if (meshfile.meshes.empty()) {
      meshfile.meshes.resize(1);
    } 

    // Update the last geometry in the buffer.
    auto &raw = meshfile.meshes.back();

    // VERTEX
    if (s[0] == 'v') {
      if (s[1] == ' ')
      {
        Vec3f v;
        sscanf(s+2, "%f %f %f", &v.x, &v.y, &v.z);
        raw.vertices.push_back(v);
      }
      else if (s[1] == 't')
      {
        Vec2f v;
        sscanf(s+3, "%f %f", &v.x, &v.y);
        raw.texcoords.push_back(v);
      }
      else //if (s[1] == 'n')
      {
        Vec3f v;
        sscanf(s+3, "%f %f %f", &v.x, &v.y, &v.z);
        raw.normals.push_back(v);
      }
    }
    // FACE
    else if (s[0] == 'f') {
      // vertex attribute indices, will be set to 0 (-1 after postprocessing)
      // if none exists.
      Vec3i f(0), ft(0), fn(0);

      // This vector is fed by the 4th extra attribute, if any, to perform
      // auto-triangularization for quad faces.
      Vec3i ext(0), neo(0);

      bool const has_texcoords = !raw.texcoords.empty();
      bool const has_normals   = !raw.normals.empty();

      if (has_texcoords && has_normals)
      {
        sscanf(s+2, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &f.x, &ft.x, &fn.x,
                                                           &f.y, &ft.y, &fn.y,
                                                           &f.z, &ft.z, &fn.z,
                                                           &ext.x, &ext.y, &ext.z);
      }
      else if (has_texcoords)
      {
        sscanf(s+2, "%d/%d %d/%d %d/%d %d/%d", &f.x, &ft.x,
                                               &f.y, &ft.y,
                                               &f.z, &ft.z,
                                               &ext.x, &ext.y);
      }
      else if (has_normals)
      {
        sscanf(s+2, "%d//%d %d//%d %d//%d %d//%d", &f.x, &fn.x,
                                                   &f.y, &fn.y,
                                                   &f.z, &fn.z,
                                                   &ext.x, &ext.z);
      }
      else // only vertex ids
      {
        sscanf(s+2, "%d %d %d %d", &f.x, &f.y, &f.z, &ext.x);
      }

      raw.elementsAttribs.push_back( Vec3i(f.x, ft.x, fn.x) );
      raw.elementsAttribs.push_back( Vec3i(f.y, ft.y, fn.y) );
      raw.elementsAttribs.push_back( Vec3i(f.z, ft.z, fn.z) );
      if (ext.x > 0) {
        raw.elementsAttribs.push_back( Vec3i(f.z, ft.z, fn.z) );
        raw.elementsAttribs.push_back( Vec3i(ext.x, ext.y, ext.z) );
        raw.elementsAttribs.push_back( Vec3i(f.x, ft.x, fn.x) );
      }
    }
  }

  // Post-process mesh with indices.
  for (auto &raw : meshfile.meshes) {
    if (raw.elementsAttribs.empty()) {
      continue;
    }

    // Change range of indices from [1, n] to [0, n-1].
    for (auto &v : raw.elementsAttribs) { 
      v.x -= 1; 
      v.y -= 1; 
      v.z -= 1; 
    }
  
    // Create a default vertex group if none were specified.
    if (!raw.has_vertex_groups()) {
      raw.vgroups.resize(1);
      raw.vgroups[0].name = MeshData::kDefaultGroupName;
    }

    // Update border groups indices.
    auto &vgroups = raw.vgroups;
    vgroups.front().start_index = 0;
    vgroups.back().end_index = static_cast<int32_t>(raw.elementsAttribs.size()) - 1;
  }
}

void ParseMTL(char *input, MaterialFile &matfile) {
  // Check if a string match a token.
  auto check_token = [](std::string_view s, std::string_view token) {
    return 0 == strncmp( s.data(), token.data(), token.size());
  };

  auto &materials = matfile.infos;
  char namebuffer[128]{'\0'};

  char *s = input;
  for (char *end = strchr(s, '\n'); end != nullptr; s=end+1u, end = strchr(s, '\n')) //
  {
    *end = '\0';
    
    // New material.
    if (s[0] == 'n') {
      materials.resize(materials.size() + 1);
      sscanf( s+7, "%128s", namebuffer);
      materials.back().name = std::string(namebuffer);
      continue;
    }

    auto &mat = materials.back();

    if (!(s[0] & 1)) {
      continue;
    }
    
    if ((s[0] == 'm') && (s[1] == 'a') && (s[2] == 'p'))
    {
      if (check_token(s, "map_Kd")) {
        sscanf( s+7, "%128s", namebuffer);
        mat.diffuse_map = std::string(namebuffer);
      } else if (check_token(s, "map_Ks")) {
        sscanf( s+7, "%128s", namebuffer);
        mat.specular_map = std::string(namebuffer);
      } else if (check_token(s, "map_Bump -bm")) {
        float tmp;
        sscanf( s+13, "%f %128s", &tmp, namebuffer);
        mat.bump_map = std::string( namebuffer );
        // (sometimes the bump might have a scale factor we bypass)
        LOG_WARNING("[MTL Loader] Bypassed bump factor", tmp, "for file :\n", namebuffer);
      } else if (check_token(s, "map_Bump")) {
        sscanf( s+9, "%128s", namebuffer);
        mat.bump_map = std::string( namebuffer );
      } else if (check_token(s, "map_d")) {
        sscanf( s+6, "%128s", namebuffer);
        mat.alpha_map = std::string( namebuffer );
      }
    }
    // Colors.
    else if (s[0] == 'K')
    {
      glm::vec3 v;
      sscanf(s+2, "%f %f %f", &v.x, &v.y, &v.z);

      // Diffuse.
      if (s[1] == 'd') {
        mat.diffuse_color.x = v.x;
        mat.diffuse_color.y = v.y;
        mat.diffuse_color.z = v.z;
      }
      // Specular.
      else if (s[1] == 's') {
        mat.specular_color.x = v.x;
        mat.specular_color.y = v.y;
        mat.specular_color.z = v.z;
      }
    }
    // Alpha value.
    else if (s[0] == 'd') {
      sscanf(s+2, "%f", &mat.diffuse_color.w);
    }
    // Specular exponent.
    else if ((s[0] == 'N') && (s[1] == 's')) {
      sscanf(s+3, "%f", &mat.specular_color.w);
    } 
  }
}

} // namespace

// ----------------------------------------------------------------------------

MeshDataManager::Handle MeshDataManager::_load(ResourceId const& id) {
  MeshDataManager::Handle h(id);

  auto const path = id.path;
  auto const ext  = path.substr(path.find_last_of(".") + 1);

  if ("obj" == ext) {
    load_obj( path, *h.data);
  } else {
    LOG_WARNING( ext, "meshes are not supported." );
  }

  return h;
}

// ----------------------------------------------------------------------------

bool MeshDataManager::load_obj(std::string_view filename, MeshData &meshdata) {
  char *buffer = nullptr;
  size_t buffersize = 0uL;

  if (!LoadFile(filename, &buffer, &buffersize)) {
    return false;
  }

  // (to prevent expecting comas instead of points when parsing floating point values)
  //char *locale = std::setlocale(LC_ALL, nullptr);
  //std::setlocale(LC_ALL, "C");

  constexpr bool bSplitObjects = false; //

  // Parse the OBJ for raw mesh data.
  RawMeshFile meshfile;
  ParseOBJ(buffer, meshfile, bSplitObjects);

  // Handle MTL file materials if any.
  if (!meshfile.material_id.empty()) {
    constexpr bool debug_log = true;

    // Determine the material file full path.
    std::string fn( filename );
    // OBJ mesh directory name.
    std::string dirname = fn.substr(0, fn.find_last_of('/'));
    // Add the material relative path.
    fn = dirname + "/" + meshfile.material_id;

    // Trim material id.
    auto &mtl = meshdata.material;
    mtl.id = meshfile.material_id;
    mtl.id = mtl.id.substr(0, mtl.id.find_last_of('.'));

    // Load & Parse the material file.
    if (LoadFile(fn, &buffer, &buffersize)) {
      ParseMTL( buffer, mtl);
    }

    // Prefix VertexGroups's and materials names by the main material id to avoid collisions.
    for (auto &mesh : meshfile.meshes) {
      for (auto &vg : mesh.vgroups) {
        vg.name = mtl.id + "::" + vg.name;
      }
    }
    for (auto &mat : mtl.infos) {
      mat.name = mtl.id + "::" + mat.name;
    }

    // Transform relative paths to absolute ones.
    for (auto &mat : mtl.infos) {
      if (auto &m = mat.diffuse_map; !m.empty() && m[0]!= '/')  m = dirname + "/" + m;
      if (auto &m = mat.specular_map; !m.empty() && m[0]!= '/') m = dirname + "/" + m;
      if (auto &m = mat.bump_map; !m.empty() && m[0]!= '/')     m = dirname + "/" + m;
      if (auto &m = mat.alpha_map; !m.empty() && m[0]!= '/')    m = dirname + "/" + m;
    }

    // Display a debug log.
    if constexpr (debug_log) {
      LOG_INFO( "\nMTL :", mtl.id );
      // for (auto &vg : meshfile.meshes[0].vgroups) {
      //   LOG_INFO( " >", vg.name );
      // }
      for (auto &mat : mtl.infos) {
        LOG_INFO(" >", mat.name);
        LOG_MESSAGE(" * Diffuse color :", mat.diffuse_color.x, mat.diffuse_color.y, mat.diffuse_color.z);
        if (auto &m = mat.diffuse_map; !m.empty())  LOG_MESSAGE(" * Diffuse  :", m);
        if (auto &m = mat.specular_map; !m.empty()) LOG_MESSAGE(" * Specular :", m);
        if (auto &m = mat.bump_map; !m.empty())     LOG_MESSAGE(" * Bump     :", m);
        if (auto &m = mat.alpha_map; !m.empty())    LOG_MESSAGE(" * Alpha    :", m);
      }
    }
  }
  delete [] buffer;

  // reset to previous locale.
  //std::setlocale(LC_ALL, locale);

  // Determine the file type (point cloud or triangle mesh).
  auto &raw = meshfile.meshes[0]; //
  MeshData::PrimitiveType type = raw.elementsAttribs.empty() ? MeshData::POINTS
                                                             : MeshData::TRIANGLES
                                                             ;
  meshdata.setup( type, raw); //

  return true;
}

// ----------------------------------------------------------------------------
