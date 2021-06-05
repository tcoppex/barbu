#ifndef BARBU_FX_MARCHING_CUBE_H_
#define BARBU_FX_MARCHING_CUBE_H_

#include <vector>

#include "core/graphics.h"
#include "memory/assets/texture.h"
#include "memory/assets/program.h"

class Camera;

// ----------------------------------------------------------------------------

class MarchingCube {
 private:
  // Number of voxel per dimension for a chunk
  static constexpr int32_t kChunkDim    = 32u;
  static constexpr float kInvChunkDim   = 1.0f / static_cast<float>(kChunkDim);

  // Chunk size in world-space
  static constexpr float kChunkSize     = 12.0f; // 
  static constexpr float kVoxelSize     = kChunkSize * kInvChunkDim;

  // Number of voxel in a slice
  static constexpr int32_t kVoxelsPerSlice = kChunkDim * kChunkDim;

  // We need to use a marge for algorithm looking on border chunk, like normal calculation.
  static constexpr int32_t kMargin       = 1u;
  static constexpr int32_t kWindowDim    = kChunkDim + 2u * kMargin;
  static constexpr float kInvWindowDim   = 1.0f / static_cast<float>(kWindowDim);

  // Density volume resolution (# of voxel corners)
  static constexpr int32_t kTextureRes   = kWindowDim + 1u;

  // Size of the batch of chunks buffer
  static constexpr int32_t kBufferBatchSize = 350u;

  // There is a maximum of 5 triangles per voxels.
  static constexpr int32_t kMaxTrianglesPerVoxel = 5u;

 public:
  MarchingCube()
   : grid_dim_{0, 0, 0}
   , nfreebuffers_(0u)
   , bInitialized_(false)
  {}

  ~MarchingCube() {
    deinit();
  }

  void init();
  void deinit();

  void generate(glm::ivec3 const& grid_dimension);
  void generate(int32_t grid_width, int32_t grid_height, int32_t grid_depth) {
    generate( glm::ivec3(grid_width, grid_height, grid_depth) );
  }

  void render(Camera const& camera);

 private:
  enum ChunkTriState_t {
    CHUNK_EMPTY,
    CHUNK_FILLED
  };

  struct ChunkInfo_t {
    int32_t id = -1;
    glm::ivec3 coords;        // grid position (indices)
    glm::vec3 ws_coords;      // world-space coordinates

    //float distance_from_camera;
    ChunkTriState_t state;
  };

  void init_textures();
  void init_buffers();
  void init_shaders();

  void create_chunk( glm::ivec3 const& coords );
  void build_density_volume(ChunkInfo_t &chunk);
  void list_triangles();
  void generate_vertices(ChunkInfo_t &chunk);

  std::vector<ChunkInfo_t> grid_;
  glm::ivec3 grid_dim_;

  struct {
    ProgramHandle build_density;
    ProgramHandle trilist;
    ProgramHandle genvertices;
    ProgramHandle render_chunk;
  } programs_;

  TextureHandle density_tex_;
  
  //-------------------

  struct {
    GLuint vao;
    GLuint in_vbo;
    GLuint out_vbo;
    GLuint tf;
    GLuint query = 0u;
  } trilist_;

  // TBOs
  struct {
    GLuint lut_tex;
    GLuint lut_vbo;

    GLuint edge_connect_tex;
    GLuint edge_connect_vbo;
  } tbo_;

  std::vector<GLuint> tf_stack_;
  std::vector<GLuint> debug_vbos_;

  GLuint genvertices_vao_ = 0;
  //-------------------

  std::vector<int32_t> freebuffer_indices_;
  int32_t nfreebuffers_;
  
  bool bInitialized_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_MARCHING_CUBE_H_
