#include "memory/resources/mesh_data.h"

#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cmath>
#include <array>
#include <sstream>
#include <map>
#include <unordered_map>

#include "glm/glm.hpp"    // abs
#include "glm/gtc/type_ptr.hpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include "memory/assets/assets.h" // wip

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

bool MeshData::setup(PrimitiveType _type, RawMeshData &_raw) {
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
  }
  else 
  {
    // Recalculate normals when none exists.
    if (_raw.normals.empty() && !_raw.elementsAttribs.empty()) {
      _raw.recalculate_normals();
    }

    // Reindexing vertices from sparse input.
    std::vector<glm::ivec3> attribIndices;
    attribIndices.reserve(_raw.vertices.size());
    indices.reserve(_raw.elementsAttribs.size());
    {
      std::map<glm::ivec3, size_t, VecOrdering_t<glm::ivec3>> mapVertices;
      
      for (auto const& elem : _raw.elementsAttribs) {
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
      vertices[i].position = _raw.vertices[index.x];
      // (texcoords)
      if (index.y >= 0) {
        vertices[i].texcoord = _raw.texcoords[index.y];
      }
      // (normals)
      if (index.z >= 0) {
        vertices[i].normal = _raw.normals[index.z];
      }
    }
  }

  return true;
}

bool MeshData::setup(RawMeshFile &meshfile) {
  // DEVNOTE : for now we process only a single mesh, but in the future we will
  //           handle all sub objects from a file.

  auto &raw = meshfile.meshes[0]; //
  MeshData::PrimitiveType primtype = raw.elementsAttribs.empty() ? MeshData::POINTS
                                                                 : MeshData::TRIANGLES
                                                                 ;
  return setup( primtype, raw);
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
  
  bool const succeed = (rbytes == filesize) || (feof(fd) && !ferror(fd));
  fclose(fd);

  if (!succeed) {
    LOG_ERROR( "Failed to load file :", filename);
    delete [] *buffer;
    return false;
  }
  
  return succeed;
}

// This method parses all the geometry and fed 'raw' in one pass.
// It is destructive of the input buffer but should always works,
// note however that the data could be easily restored after each iteration.
void ParseOBJ(char *input, RawMeshFile &meshfile, bool bSeparateObjects) {
  using Vec2f = glm::vec2;
  using Vec3f = glm::vec3;
  using Vec3i = glm::ivec3;

  char namebuffer[128]{'\0'};

  // If no objects are specified, create a default raw geo.
  if (meshfile.meshes.empty()) {
    meshfile.meshes.resize(1);
  } 

  char *s = input;
  for (char *end = strchr(s, '\n'); end != nullptr; s = end+1u, end = strchr(s, '\n'))
  {
    *end = '\0';

    // Handle Objects, Submesh & Material file.
    if (s[0] & 1) {
      // OBJECT

      if (s[0] == '#') {
        continue;
      }

      if ((s[0] == 'o')) {//} || (s[0] == 'g')) {

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
        v.y = 1.0f - v.y; // (reverse y)
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
    vgroups.back().end_index = static_cast<int32_t>(raw.elementsAttribs.size());
  }
}

void ParseMTL(char *input, MaterialFile &matfile) {
  // Check if a string match a token.
  auto check_token = [](std::string_view s, std::string_view token) {
    return 0 == strncmp( s.data(), token.data(), token.size());
  };

  auto &materials = matfile.infos;
  char namebuffer[128]{'\0'};

  // For MTL, as there is no way to determine alpha testing / blending, we
  // decide that materials are :
  //  * opaque by default, 
  //  * alpha_tested if they've got a map_Kd (with a potential alpha value) or an map_d, 
  //  * transparent if they've got an opacity value ('d').

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
        mat.bAlphaTest = true;
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
        mat.bAlphaTest = true;
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
      mat.bBlending = true;
    }
    // Specular exponent.
    else if ((s[0] == 'N') && (s[1] == 's')) {
      sscanf(s+3, "%f", &mat.specular_color.w);
    } 
  }
}

} // namespace

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool MeshDataManager::CheckExtension(std::string_view _ext) {
  std::string ext(_ext);
  std::transform( _ext.begin(), _ext.end(), ext.begin(), ::tolower);
  
  return ("obj" == ext)
      || ("glb" == ext)
      || ("gltf" == ext)
      ;  
}

// ----------------------------------------------------------------------------

MeshDataManager::Handle MeshDataManager::_load(ResourceId const& id) {
  MeshDataManager::Handle h(id);

  auto const path = id.path;
  
  auto ext = path.substr(path.find_last_of(".") + 1);
  std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower);

  auto &meshdata = *h.data;

  if ("obj" == ext) {
    load_obj( path, meshdata);
  } else if (("glb" == ext) || ("gltf" == ext)) {
    load_gltf( path, meshdata);
  } else {
    LOG_WARNING( ext, "models are not supported." );
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

#ifdef WIN32
  // (to prevent expecting comas instead of points when parsing floating point values)
  char *locale = std::setlocale(LC_ALL, nullptr);
  std::setlocale(LC_ALL, "C");
#endif

  constexpr bool bSplitObjects = false; //

  // Parse the OBJ for raw mesh data.
  RawMeshFile meshfile;
  ParseOBJ(buffer, meshfile, bSplitObjects);

  // Handle MTL file materials if any.
  if (!meshfile.material_id.empty()) {
    constexpr bool debug_log = false;

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
    meshfile.prefix_material_vg_names(mtl);

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

#ifdef WIN32
  // reset to previous locale.
  std::setlocale(LC_ALL, locale);
#endif

  return meshdata.setup( meshfile ); //
}

// ----------------------------------------------------------------------------

void DisplayStats_GLTF(cgltf_data* data) {
  assert( data->scenes_count > 0 );
  assert( data->nodes_count > 0 );

  LOG_INFO( "Scenes :     ", data->scenes_count    );
  LOG_INFO( "Nodes :      ", data->nodes_count     );
  LOG_INFO( "Meshes :     ", data->meshes_count    );
  LOG_INFO( "Skins :      ", data->skins_count     );
  LOG_INFO( "Materials :  ", data->materials_count );
  LOG_INFO( "Textures :   ", data->textures_count  );

  //LOG_MESSAGE( "", "ia->offset", "ia->count", "ia->stride", "data_view->offset", "data_view->size");
  for (cgltf_size i=0; i < data->nodes_count; ++i) {
    auto node = data->nodes[i];
    LOG_INFO( "* Node ", node.name );

    auto *mesh = node.mesh;
    if (!mesh) {
      continue;
    }

    if (mesh->name) {
      LOG_MESSAGE( "  ", "Mesh name : ", mesh->name );
    }

    // Primitive are actual submesh, with specific material.
    LOG_MESSAGE( "  ", "Mesh primitives : ", mesh->primitives_count );
    LOG_MESSAGE("");
    for (cgltf_size j=0; j < mesh->primitives_count; ++j) {
      auto prim = mesh->primitives[j];  
      
      // if (prim.material) {
      //   LOG_MESSAGE( "  * material : ", prim.material->name);
      // }
      if (prim.indices) LOG_MESSAGE( "  * indices : ", prim.indices->component_type);

      for (cgltf_size attrib_index = 0; attrib_index < prim.attributes_count; ++attrib_index) { 
        LOG_MESSAGE( "  * attribs : ", prim.attributes[attrib_index].name);
      }
      // auto ia = prim.indices;
      // LOG_MESSAGE( "  ", prim.material->name, ia->offset, ia->count, ia->stride, ia->buffer_view->offset, ia->buffer_view->size);
      
      // auto buf = ia->buffer_view->buffer;
      // LOG_MESSAGE( "  ", buf->size, buf->data );
      
      LOG_MESSAGE("");
    }
  }
}

std::string setup_gltf_texture(cgltf_texture *tex, std::string const& dirname, std::string const &default_name) {
  std::string texname;

  if (!tex) {
    return texname;
  }

  auto img = tex->image;
 
  if (img->uri) {
    // GLTF file with external data.
    
    if (img->uri[0] != '/') {
      texname = dirname + "/" + std::string(img->uri);
    } else {
      texname = std::string(img->uri);
    }
  } else {
    // GLB / GLTF file with internal data.

    texname = img->name ? img->name : default_name; //
    if (img->buffer_view) {
      auto buffer_view = img->buffer_view;

      // Create the resource internally.
      Resources::LoadInternal<Image>( 
        ResourceId(texname), 
        buffer_view->size, 
        ((uint8_t*)buffer_view->buffer->data) + buffer_view->offset,
        img->mime_type
      ); 
      // [optional] Create the texture directly.
      //TEXTURE_ASSETS.create2d(AssetId(texname)); 
    }
  }

  return texname;
}


bool MeshDataManager::load_gltf(std::string_view filename, MeshData &meshdata) {
  cgltf_options options{};
  cgltf_data* data = nullptr;
  cgltf_result result = cgltf_parse_file(&options, filename.data(), &data);
  
  if (result != cgltf_result_success) {
    LOG_WARNING( filename, "failed to parse.");
    return false;
  }

  // Load buffers data.
  result = cgltf_load_buffers(&options, data, filename.data());

  if (result != cgltf_result_success) {
    LOG_WARNING( filename, "failed to load buffers.");
    return false;
  }

  std::string basename(filename);
  basename = Resource::TrimFilename(basename);
  basename = basename.substr(0, basename.find_last_of('.'));

  /* ------------ */
  RawMeshFile meshfile;
  {
    // We will join all meshes as a single one.
    if (meshfile.meshes.empty()) {
      meshfile.meshes.resize(1);
    }

    char tmpname[256]{};

    // Map to solve name for unknown materials.
    std::unordered_map< cgltf_material const*, std::string > material_names( data->materials_count );
    for (cgltf_size i = 0; i < data->materials_count; ++i) {
      cgltf_material const& mat = data->materials[i];
      sprintf(tmpname, "%s::material_%02d", basename.c_str(), int(i));
      material_names[ &mat ] = std::string( mat.name ? mat.name : tmpname );
    }

    // -- MESH ATTRIBUTES & INDICES.
    cgltf_size last_vertex_index = 0;
    for (cgltf_size i=0; i < data->nodes_count; ++i) {
      auto node = data->nodes[i];
      auto *mesh = node.mesh;

      if (!mesh) {
        continue;
      }

      // Node's transform.
      glm::mat4 world_matrix;
      cgltf_node_transform_world( &node, glm::value_ptr(world_matrix));

      // RawMeshData to update.
      auto &raw = meshfile.meshes.back();
      sprintf(tmpname, "%s::mesh_%02d", basename.c_str(), int(i));
      raw.name = std::string(mesh->name ? mesh->name : node.name ? node.name : tmpname); //

      // When meshes are not joined, we should probably reset this to 0.
      //last_vertex_index = 0;

      // Primitives / submeshes.
      for (cgltf_size j = 0; j < mesh->primitives_count; ++j) {
        auto const& prim = mesh->primitives[j];

        if (prim.has_draco_mesh_compression) {
          LOG_WARNING( "GLTF Draco mesh compression is not supported." );
          continue;
        }

        if (prim.type != cgltf_primitive_type_triangles) {
          LOG_WARNING( "GLTF non TRIANGLES primitives are not implemented : ", raw.name );
        }

        // Attributes.
        for (cgltf_size attrib_index = 0; attrib_index < prim.attributes_count; ++attrib_index) {
          auto const& attrib = prim.attributes[attrib_index];

          // Check accessors layout.
          if (attrib.data->is_sparse) {
            LOG_WARNING( "GLTF sparse attributes are not supported." );
            continue;
          }
          //LOG_MESSAGE("Accessor type :", attrib.data->component_type);

          // Used to reserve memory.
          auto const attribsize = attrib.data->count;

          // Postions.
          if (attrib.type == cgltf_attribute_type_position) {
            raw.vertices.reserve(attribsize);
            
            glm::vec3 vertex;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(vertex), 3);
              vertex = glm::vec3(world_matrix * glm::vec4(vertex, 1.0f));
              raw.vertices.push_back( vertex );
            }
          }
          // Normals.
          else if (attrib.type == cgltf_attribute_type_normal) {
            raw.normals.reserve(attribsize);
            
            glm::vec3 normal;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(normal), 3);
              normal = glm::vec3(world_matrix * glm::vec4(normal, 0.0f));
              raw.normals.push_back( normal );
            }
          }
          // Texcoords.
          else if (attrib.type == cgltf_attribute_type_texcoord) {
            raw.texcoords.reserve(attribsize);
            
            glm::vec2 texcoord;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(texcoord), 2);
              raw.texcoords.push_back( texcoord );
            }
          }
          // Joints.
          else if (attrib.type == cgltf_attribute_type_joints) {
            raw.joints.reserve(attribsize);
            
            glm::uvec4 joints;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_uint( attrib.data, index, glm::value_ptr(joints), 4);
              raw.joints.push_back( joints );
            }
          }
          // Weights.
          else if (attrib.type == cgltf_attribute_type_weights) {
            raw.weights.reserve(attribsize);

            glm::vec4 weights;
            for (cgltf_size index = 0; index < attrib.data->count; ++index) {
              cgltf_accessor_read_float( attrib.data, index, glm::value_ptr(weights), 4);
              raw.weights.push_back( weights );
            }
          }
        }
        
        // Indices.
        if (prim.indices) {
          if (prim.indices->is_sparse) {
            LOG_WARNING( "GLTF sparse indexing is not implemented." );
          } else {
            for (cgltf_size index = 0; index < prim.indices->count; ++index) {
              auto const vid = cgltf_accessor_read_index(prim.indices, index);
              raw.elementsAttribs.push_back(glm::ivec3( last_vertex_index + vid ));
            }
          }

          // Vertex Group.
          if (auto mat = prim.material; mat) {
            VertexGroup vg;

            vg.name = material_names[mat];
            vg.start_index = raw.elementsAttribs.size() - prim.indices->count; // 
            vg.end_index   = raw.elementsAttribs.size(); //
            raw.vgroups.push_back(vg);
          }
        } else {
          LOG_WARNING( "No indices are associated with GLTF file :", filename );
        }

        last_vertex_index = raw.vertices.size(); //
      }

      // Skeleton datas.
      if (auto *skin = node.skin; skin) {
        auto const njoints = skin->joints_count;

        meshdata.skeleton = std::make_shared<Skeleton>( njoints );
        auto &skl = meshdata.skeleton;

        // Map to find (parent) nodes index.
        std::unordered_map< cgltf_node*, int32_t> joint_indices( njoints );
        for (cgltf_size index = 0; index < njoints; ++index) {
          auto *joint = skin->joints[index];
          joint_indices[ joint ] = index;
        }
        
        // Fill skeleton data.
        for (cgltf_size index = 0; index < njoints; ++index) {
          auto *joint = skin->joints[index];

          // Joint name.
          sprintf(tmpname, "%s::joint_%02d", basename.c_str(), int(index)); //
          skl->names.push_back( (joint->name) ? joint->name : tmpname );

          // Joint parent index.
          auto const& it = joint_indices.find( joint->parent );
          skl->parents.push_back( (it != joint_indices.end()) ? it->second : -1 );
        }

        // Fill the inverse bind matrix buffer.
        cgltf_accessor_unpack_floats( skin->inverse_bind_matrices, glm::value_ptr(*skl->inverse_bind_matrices.data()), 16 * njoints); //
      }
    }

    // -- MATERIALS.
    auto &mtl = meshdata.material;
    std::string const fn( filename );
    std::string const dirname = fn.substr(0, fn.find_last_of('/'));

    for (cgltf_size i = 0; i < data->materials_count; ++i) {
      auto const& mat = data->materials[i];
      
      MaterialInfo info;
      info.name = material_names[ &mat ];
      if (mat.has_pbr_metallic_roughness) {
        auto const pmr = mat.pbr_metallic_roughness;

        auto const rgba = pmr.base_color_factor;
        info.diffuse_color = glm::vec4(rgba[0], rgba[1], rgba[2], rgba[3]);        
        info.metallic  = pmr.metallic_factor;
        info.roughness = pmr.roughness_factor;

        info.diffuse_map        = setup_gltf_texture( pmr.base_color_texture.texture, dirname, info.name + "_diffuse");
        info.metallic_rough_map = setup_gltf_texture( pmr.metallic_roughness_texture.texture, dirname, info.name + "_metallic_rough");
      }

      switch (mat.alpha_mode) {
        case cgltf_alpha_mode_blend:
          info.bBlending = true;
        break;
        case cgltf_alpha_mode_mask:
          info.bAlphaTest = true;
        break;
        case cgltf_alpha_mode_opaque:
        default:
        break;
      }

      info.ao_map       = setup_gltf_texture( mat.occlusion_texture.texture, dirname, info.name + "_occlusion");
      info.alpha_cutoff = mat.alpha_cutoff;

      info.bDoubleSided = mat.double_sided;
      info.bUnlit       = mat.unlit;

      mtl.infos.push_back(info);
    }

    // Rename materials and vertex group ids.
    meshfile.material_id = fn; 
    mtl.id = fn.substr(fn.find_last_of('/') + 1);
    mtl.id = mtl.id.substr(0, mtl.id.find_last_of('.'));
    meshfile.prefix_material_vg_names(mtl);
  
    // -- ANIMATION DATAS.
    for (cgltf_size i=0; i < data->animations_count; ++i) {
      LOG_WARNING( "GLTF animation data is not supported yet." );
    }
  }

  /* ------------ */

  cgltf_free(data);

  return meshdata.setup( meshfile ); //
}

// ----------------------------------------------------------------------------
