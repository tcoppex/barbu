#include "fx/marching_cube.h"

#include <array>

#include "core/camera.h"
#include "core/global_clock.h"
#include "memory/assets/assets.h"

#define USE_COMPUTE_SHADERS 1

// ----------------------------------------------------------------------------

static constexpr int8_t s_caseToNumpolys[]{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 2, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 
  3, 4, 3, 4, 4, 3, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 2, 
  3, 4, 4, 3, 3, 4, 4, 3, 4, 5, 5, 2, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 
  4, 3, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4, 2, 3, 3, 4, 3, 4, 2, 3, 
  3, 4, 4, 5, 4, 5, 3, 2, 3, 4, 4, 3, 4, 5, 3, 2, 4, 5, 5, 4, 5, 2, 4, 1, 1, 2, 
  2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 4, 3, 4, 4, 5, 3, 2, 4, 3, 
  4, 3, 5, 2, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4, 3, 4, 4, 3, 4, 5, 
  5, 4, 4, 3, 5, 2, 5, 4, 2, 1, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 2, 3, 3, 2, 
  3, 4, 4, 5, 4, 5, 5, 2, 4, 3, 5, 4, 3, 2, 4, 1, 3, 4, 4, 5, 4, 5, 3, 4, 4, 5, 
  5, 2, 3, 4, 2, 1, 2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 1, 1, 0
};

static constexpr int16_t s_packedEdgesIndicesPerCase[]{
  0, 0, 0, 0, 0, 
  896, 0, 0, 0, 0, 
  2320, 0, 0, 0, 0, 
  897, 393, 0, 0, 0, 
  2593, 0, 0, 0, 0, 
  896, 2593, 0, 0, 0, 
  2601, 2336, 0, 0, 0, 
  898, 2210, 2202, 0, 0, 
  691, 0, 0, 0, 0, 
  688, 184, 0, 0, 0, 
  145, 2866, 0, 0, 0, 
  689, 2961, 2953, 0, 0, 
  419, 939, 0, 0, 0, 
  416, 2688, 2744, 0, 0, 
  147, 2483, 2475, 0, 0, 
  2697, 2954, 0, 0, 0, 
  2164, 0, 0, 0, 0, 
  52, 1079, 0, 0, 0, 
  2320, 1864, 0, 0, 0, 
  2324, 372, 311, 0, 0, 
  2593, 1864, 0, 0, 0, 
  1859, 1027, 2593, 0, 0, 
  2601, 521, 1864, 0, 0, 
  2466, 1938, 882, 1175, 0, 
  1864, 691, 0, 0, 0, 
  1867, 1067, 1026, 0, 0, 
  265, 1864, 2866, 0, 0, 
  2932, 2889, 697, 297, 0, 
  419, 2739, 1159, 0, 0, 
  2737, 2881, 1025, 1207, 0, 
  2164, 2825, 2745, 779, 0, 
  2932, 2484, 2745, 0, 0, 
  1113, 0, 0, 0, 0, 
  1113, 896, 0, 0, 0, 
  1104, 81, 0, 0, 0, 
  1112, 1336, 1299, 0, 0, 
  2593, 1113, 0, 0, 0, 
  2051, 2593, 1428, 0, 0, 
  2597, 581, 516, 0, 0, 
  1442, 1315, 1107, 2115, 0, 
  1113, 2866, 0, 0, 0, 
  688, 2944, 1428, 0, 0, 
  1104, 1296, 2866, 0, 0, 
  1298, 2130, 2946, 1412, 0, 
  2874, 794, 1113, 0, 0, 
  1428, 384, 424, 2744, 0, 
  69, 2821, 2741, 779, 0, 
  2117, 2693, 2954, 0, 0, 
  2169, 2421, 0, 0, 0, 
  57, 857, 885, 0, 0, 
  2160, 1808, 1873, 0, 0, 
  849, 1875, 0, 0, 0, 
  2169, 1881, 538, 0, 0, 
  538, 89, 53, 885, 0, 
  520, 1320, 1880, 602, 0, 
  1442, 850, 1875, 0, 0, 
  1431, 2439, 691, 0, 0, 
  1881, 633, 41, 2930, 0, 
  2866, 2064, 2161, 1873, 0, 
  299, 1819, 1303, 0, 0, 
  2137, 1880, 794, 2874, 0, 
  117, 2309, 183, 2561, 171, 
  171, 779, 90, 1800, 117, 
  1451, 1463, 0, 0, 0, 
  1386, 0, 0, 0, 0, 
  896, 1701, 0, 0, 0, 
  265, 1701, 0, 0, 0, 
  897, 2193, 1701, 0, 0, 
  1377, 354, 0, 0, 0, 
  1377, 1569, 2051, 0, 0, 
  1385, 1545, 1568, 0, 0, 
  2197, 645, 1573, 2083, 0, 
  2866, 1386, 0, 0, 0, 
  2059, 43, 1386, 0, 0, 
  2320, 2866, 1701, 0, 0, 
  1701, 657, 697, 2953, 0, 
  2870, 854, 789, 0, 0, 
  2944, 1456, 336, 1717, 0, 
  1715, 1584, 1376, 2384, 0, 
  2390, 2966, 2203, 0, 0, 
  1701, 2164, 0, 0, 0, 
  52, 884, 2646, 0, 0, 
  145, 1701, 1864, 0, 0, 
  1386, 1937, 881, 1175, 0, 
  534, 342, 2164, 0, 0, 
  1313, 1573, 1027, 1859, 0, 
  1864, 1289, 1376, 1568, 0, 
  2359, 1175, 2339, 1685, 2402, 
  691, 1159, 1386, 0, 0, 
  1701, 628, 36, 2930, 0, 
  2320, 2164, 2866, 1701, 0, 
  297, 697, 2889, 1207, 1701, 
  1864, 1459, 339, 1717, 0, 
  2837, 1717, 2817, 1207, 2880, 
  2384, 1376, 1584, 875, 1864, 
  2390, 2966, 2420, 2487, 0, 
  2378, 2630, 0, 0, 0, 
  1700, 2708, 896, 0, 0, 
  266, 106, 70, 0, 0, 
  312, 1560, 1128, 2582, 0, 
  2369, 1057, 1122, 0, 0, 
  2051, 2337, 2370, 1122, 0, 
  1056, 1572, 0, 0, 0, 
  568, 1064, 1572, 0, 0, 
  2378, 1130, 811, 0, 0, 
  640, 2946, 2708, 1700, 0, 
  691, 1552, 1120, 2582, 0, 
  326, 2582, 388, 2834, 440, 
  1129, 1593, 793, 875, 0, 
  440, 24, 363, 1049, 326, 
  1715, 99, 1120, 0, 0, 
  2118, 2155, 0, 0, 0, 
  1703, 2695, 2712, 0, 0, 
  880, 1952, 2704, 2678, 0, 
  1898, 1953, 2161, 129, 0, 
  1898, 378, 881, 0, 0, 
  1569, 2145, 2433, 1896, 0, 
  2402, 402, 2422, 912, 2359, 
  135, 1543, 518, 0, 0, 
  567, 630, 0, 0, 0, 
  2866, 2154, 2442, 1896, 0, 
  1794, 2930, 1936, 2678, 1961, 
  129, 2161, 1953, 2678, 2866, 
  299, 1819, 362, 374, 0, 
  1688, 1896, 1561, 875, 1585, 
  400, 1899, 0, 0, 0, 
  135, 1543, 179, 107, 0, 
  1719, 0, 0, 0, 0, 
  2919, 0, 0, 0, 0, 
  2051, 1659, 0, 0, 0, 
  2320, 1659, 0, 0, 0, 
  2328, 312, 1659, 0, 0, 
  538, 1974, 0, 0, 0, 
  2593, 2051, 1974, 0, 0, 
  146, 2466, 1974, 0, 0, 
  1974, 930, 906, 2202, 0, 
  807, 1830, 0, 0, 0, 
  2055, 103, 38, 0, 0, 
  1650, 1842, 2320, 0, 0, 
  609, 1665, 2193, 1656, 0, 
  1658, 1818, 1841, 0, 0, 
  1658, 2673, 1921, 2049, 0, 
  1840, 2672, 2464, 1958, 0, 
  2663, 2215, 2472, 0, 0, 
  1158, 1675, 0, 0, 0, 
  2915, 1539, 1600, 0, 0, 
  2920, 1608, 265, 0, 0, 
  1609, 873, 313, 1595, 0, 
  1158, 2230, 418, 0, 0, 
  2593, 2819, 2912, 1600, 0, 
  2228, 2916, 2336, 2466, 0, 
  922, 570, 841, 1595, 868, 
  808, 584, 612, 0, 0, 
  576, 612, 0, 0, 0, 
  145, 1074, 1602, 2100, 0, 
  1169, 577, 1602, 0, 0, 
  792, 360, 1608, 422, 0, 
  26, 1546, 1030, 0, 0, 
  868, 2100, 934, 2352, 922, 
  1178, 1190, 0, 0, 0, 
  1428, 2919, 0, 0, 0, 
  896, 1428, 1659, 0, 0, 
  261, 69, 2919, 0, 0, 
  1659, 1080, 1107, 1299, 0, 
  1113, 538, 2919, 0, 0, 
  1974, 2593, 896, 1428, 0, 
  2919, 2629, 2596, 516, 0, 
  2115, 1107, 1315, 602, 1659, 
  807, 615, 2373, 0, 0, 
  1113, 1664, 608, 1926, 0, 
  611, 1651, 81, 69, 0, 
  2086, 1926, 2066, 1412, 2129, 
  1113, 1562, 1649, 1841, 0, 
  2657, 1649, 1793, 120, 1113, 
  2564, 1444, 2608, 1958, 2675, 
  2663, 2215, 2629, 2692, 0, 
  1430, 2486, 2443, 0, 0, 
  2915, 864, 1616, 1424, 0, 
  2224, 2896, 1296, 2917, 0, 
  950, 1334, 309, 0, 0, 
  2593, 2905, 2233, 1627, 0, 
  944, 2912, 1680, 2405, 2593, 
  1419, 1627, 1288, 602, 1312, 
  950, 1334, 930, 858, 0, 
  2437, 2085, 613, 643, 0, 
  1625, 105, 608, 0, 0, 
  2129, 129, 2149, 643, 2086, 
  1617, 1554, 0, 0, 0, 
  1585, 2657, 1667, 2405, 1688, 
  26, 1546, 89, 101, 0, 
  2096, 2661, 0, 0, 0, 
  1626, 0, 0, 0, 0, 
  2651, 2903, 0, 0, 0, 
  2651, 1403, 56, 0, 0, 
  1973, 2981, 145, 0, 0, 
  1402, 1978, 393, 312, 0, 
  539, 379, 343, 0, 0, 
  896, 1825, 1393, 2855, 0, 
  1401, 1833, 521, 1970, 0, 
  599, 2855, 661, 2083, 649, 
  2642, 1330, 1395, 0, 0, 
  40, 600, 1400, 1322, 0, 
  265, 933, 1845, 675, 0, 
  649, 297, 632, 1322, 599, 
  1329, 1395, 0, 0, 0, 
  1920, 368, 1393, 0, 0, 
  777, 1337, 1845, 0, 0, 
  1929, 1941, 0, 0, 0, 
  1157, 2213, 2234, 0, 0, 
  1029, 181, 2981, 59, 0, 
  2320, 2632, 2984, 1354, 0, 
  1210, 1354, 1083, 329, 1043, 
  338, 1410, 2226, 2132, 0, 
  2880, 944, 2900, 434, 2837, 
  1312, 2384, 1458, 2132, 1419, 
  1353, 946, 0, 0, 0, 
  2642, 595, 1347, 1155, 0, 
  677, 1061, 36, 0, 0, 
  675, 2643, 1411, 2132, 2320, 
  677, 1061, 657, 585, 0, 
  1352, 856, 339, 0, 0, 
  1344, 1281, 0, 0, 0, 
  1352, 856, 1289, 1328, 0, 
  1353, 0, 0, 0, 0, 
  1972, 2964, 2985, 0, 0, 
  896, 1940, 1977, 2985, 0, 
  2977, 1201, 65, 2887, 0, 
  1043, 2115, 1185, 2887, 1210, 
  1972, 1209, 2857, 537, 0, 
  1145, 1977, 2841, 434, 896, 
  1147, 587, 66, 0, 0, 
  1147, 587, 1080, 1059, 0, 
  2706, 2418, 1842, 2375, 0, 
  1961, 1145, 1834, 120, 1794, 
  2675, 675, 2631, 161, 2564, 
  673, 1144, 0, 0, 0, 
  404, 1812, 791, 0, 0, 
  404, 1812, 384, 376, 0, 
  772, 839, 0, 0, 0, 
  1924, 0, 0, 0, 0, 
  2217, 2234, 0, 0, 0, 
  2307, 2963, 2715, 0, 0, 
  2576, 2208, 2984, 0, 0, 
  2579, 2619, 0, 0, 0, 
  2849, 2481, 2233, 0, 0, 
  2307, 2963, 2337, 2482, 0, 
  2848, 2824, 0, 0, 0, 
  2851, 0, 0, 0, 0, 
  2098, 2690, 2442, 0, 0, 
  681, 656, 0, 0, 0, 
  2098, 2690, 2064, 2209, 0, 
  673, 0, 0, 0, 0, 
  2097, 2073, 0, 0, 0, 
  400, 0, 0, 0, 0, 
  2096, 0, 0, 0, 0, 
  0, 0, 0, 0, 0
};

// ----------------------------------------------------------------------------

void MarchingCube::init() {
  assert(!bInitialized_);

  init_textures();
  init_buffers();
  init_shaders();

  bInitialized_ = true;
}

void MarchingCube::deinit() {
  /*if (!bInitialized_) {
    return;
  }

  density_tex_.release();
  density_rt_.release();

  trilist_.tf.release();
  tbo_.case_to_numtri.buffer.release();
  tbo_.case_to_numtri.texture.release();
  tbo_.edge_connect.buffer.release();
  tbo_.edge_connect.texture.release();

  for (auto &tf : tf_stack_) {
    tf.release();
  }

  //
  program_.build_density.release();
  program_.disp_density.release();
  program_.trilist.release();
  program_.genvertices.release();
  program_.render_chunk.release();

  bInitialized_ = false;*/
}

void MarchingCube::generate(glm::ivec3 const& grid_dimension) {
  assert( bInitialized_ );

  grid_dim_ = grid_dimension;
  grid_.resize(grid_dim_.x * grid_dim_.y * grid_dim_.z);

  //aer::opengl::StatesInfo gl_states = aer::opengl::PopStates();

  if constexpr (!USE_COMPUTE_SHADERS) {
    gx::Disable( gx::State::DepthTest );
    gx::DepthMask( false );
    gx::Viewport( kTextureRes, kTextureRes);
  }

  for (int k = 0; k < grid_dim_.z; ++k) {
    for (int j = 0; j < grid_dim_.y; ++j) {
      for (int i = 0; i < grid_dim_.x; ++i) {
        createChunk( glm::ivec3( i, j, k) );
      }
    }
  }

  //aer::opengl::PushStates(gl_states);
}

void MarchingCube::render(Camera const& camera) {
  assert(bInitialized_);

  //----------
  static GLuint vao = 0u;
  if (!vao) {
    glCreateVertexArrays( 1, &vao);
  }

  int32_t constexpr kVBOBinding = 0u;
  int32_t constexpr kVBOStride = 2u * sizeof(glm::vec3);
  {
    int32_t attrib = 0u;
    uintptr_t offset = 0u;

    // position
    glVertexArrayAttribBinding(vao, attrib, kVBOBinding);
    glVertexArrayAttribFormat(vao, attrib, 3, GL_FLOAT, GL_FALSE, offset);
    glEnableVertexArrayAttrib(vao, attrib++);

    // normal
    offset = 3u * sizeof(float);
    glVertexArrayAttribBinding(vao, attrib, kVBOBinding);
    glVertexArrayAttribFormat(vao, attrib, 3, GL_FLOAT, GL_FALSE, offset);
    glEnableVertexArrayAttrib(vao, attrib++);
  }
  //----------
  
  auto const pgm = programs_.render_chunk->id;
  auto const &viewproj = camera.viewproj();
  gx::SetUniform( pgm, "uMVP", viewproj);

  gx::SetUniform( pgm, "uUseAttribColor", true);

  gx::UseProgram( pgm );
  glBindVertexArray( vao );

  int32_t index = 0u;
  for (int i = 0; i < grid_dim_.x; ++i) {
    for (int j = 0; j < grid_dim_.y; ++j) {
      for (int k = 0; k < grid_dim_.z; ++k) {
        auto &chunk = grid_[index++];

        if ((chunk.id < 0) || (chunk.state != CHUNK_FILLED)) {
          continue;
        }

        //----------
        auto const& vbo = debug_vbos_[chunk.id];
        glVertexArrayVertexBuffer( vao, kVBOBinding, vbo, 0, kVBOStride); //
        //----------

        glDrawTransformFeedback( GL_TRIANGLES, tf_stack_[chunk.id]); //
      }
    }
  }

  glBindVertexArray( 0 );
  gx::UseProgram( 0 );

  CHECK_GX_ERROR();
}

void MarchingCube::init_textures() {
  // [ Try using a 2D Array instead for faster texturing fetching ]
  density_tex_ = TEXTURE_ASSETS.create3d( 
    "MarchingCube::Tex::Density", 1, GL_R32F, kTextureRes
  );

  CHECK_GX_ERROR();
}

void MarchingCube::init_buffers() {
  // Buffer used to generate the triangle list.
  // (A 2D grid of points, instanced as 'depth' layers)
  {
    // Input buffer : position of 2D points.
    {
      // Host buffer.
      std::vector<glm::ivec2> buffer(kVoxelsPerSlice);
      for (size_t i = 0; i < buffer.size(); ++i) {
        int32_t const x = i % kChunkDim; 
        int32_t const y = i / kChunkDim;
        buffer[i] = glm::ivec2( x, y);
      }
     
      // VAO.
      int32_t const kBindingIndex = 0;
      int32_t const kAttribIn     = 0;
      auto &vao = trilist_.vao;

      glCreateVertexArrays(1, &vao);
      glVertexArrayAttribBinding(vao, kAttribIn, kBindingIndex);
      glVertexArrayAttribIFormat(vao, kAttribIn, 2, GL_INT, 0);
      glEnableVertexArrayAttrib(vao, kAttribIn);

      // Device buffer.
      auto constexpr kBytesize = kVoxelsPerSlice * sizeof(glm::ivec2);
      auto &vbo = trilist_.in_vbo;
      
      glCreateBuffers( 1, &vbo);
      glNamedBufferStorage( vbo, kBytesize, buffer.data(), 0);
      glVertexArrayVertexBuffer(vao, kBindingIndex, vbo, 0, sizeof(buffer[0]));
    }

    // Output buffer containing the triangle list packed as integer.
    // (3x6bits for voxel coordinates + 3x4bits for edges indices = 30 bits per voxels)
    {
      auto &vbo = trilist_.out_vbo;

      // Device buffer.
      auto constexpr kBytesize = kMaxTrianglesPerVoxel * kChunkDim * kVoxelsPerSlice * sizeof(int32_t);
      glCreateBuffers( 1, &vbo);
      glNamedBufferStorage( vbo, kBytesize, nullptr, 0);

      // Transform Feedback.
      int32_t constexpr kAttribOutIndex = 0;
      glCreateTransformFeedbacks(1, &trilist_.tf);
      glTransformFeedbackBufferBase(trilist_.tf, kAttribOutIndex, vbo);
    }
  }

  // -----------------------------

  // Gen vertices VAO.
  {
    // VAO.
    int32_t const kBindingIndex = 0;
    int32_t const kAttribIn     = 0;
    auto &vao = genvertices_vao_;

    glCreateVertexArrays(1, &vao);
    glVertexArrayAttribBinding(vao, kAttribIn, kBindingIndex);
    glVertexArrayAttribIFormat(vao, kAttribIn, 1, GL_INT, 0);
    glEnableVertexArrayAttrib(vao, kAttribIn);

    // Device buffer.
    auto &vbo = trilist_.out_vbo;
    glVertexArrayVertexBuffer(vao, kBindingIndex, vbo, 0, sizeof(int32_t));
  }

  /// Texture Buffers
  // Setup texture buffers for LUT storing :
  // The number of triangle per voxel case (256 bytes value in [0-5])
  {
    auto constexpr kBytesize = sizeof(s_caseToNumpolys);
    auto &vbo = tbo_.lut_vbo;
    auto &tex = tbo_.lut_tex;

    glCreateBuffers(1u, &vbo);
    glNamedBufferStorage(vbo, kBytesize, s_caseToNumpolys, 0);
    
    glCreateTextures( GL_TEXTURE_BUFFER, 1u, &tex);
    glTextureBuffer( tex, GL_R8I, vbo);
  }

  // The 0 to 5 trio of edge indices where lies the triangle vertices 
  // (5*256 short value, stored as 3x4bits)
  {
    auto constexpr kBytesize = sizeof(s_packedEdgesIndicesPerCase);

    auto &vbo = tbo_.edge_connect_vbo;
    auto &tex = tbo_.edge_connect_tex;

    glCreateBuffers(1u, &vbo);
    glNamedBufferStorage(vbo, kBytesize, s_packedEdgesIndicesPerCase, 0);
    
    glCreateTextures( GL_TEXTURE_BUFFER, 1u, &tex);
    glTextureBuffer( tex, GL_R16I, vbo);
  }

  // -----------------------------

  {  
    tf_stack_.resize(kBufferBatchSize, 0);
    debug_vbos_.resize(kBufferBatchSize, 0);

    // List of free buffers [tmp]
    freebuffer_indices_.resize(kBufferBatchSize);
    nfreebuffers_ = kBufferBatchSize;

    // [TODO]
    // The total space used is not explained for now (should be large enough).
    // We use a factor of 6 to have aligned offset when using
    // bindBufferRange [see glspec43 p398]
    int constexpr kBytesize = (64u * 1024u) * (6u * sizeof(float)); //

    int32_t constexpr kTFAttribOutIndex = 0;

    // Transform feedback stacks.
    for (int i = 0u; i < kBufferBatchSize; ++i) {
      auto &tf = tf_stack_[i];
      auto &vbo = debug_vbos_[i];
      freebuffer_indices_[i] = i; //

      glCreateBuffers( 1, &vbo);
      glNamedBufferStorage( vbo, kBytesize, nullptr, GL_DYNAMIC_STORAGE_BIT);

      glCreateTransformFeedbacks(1, &tf);
      glTransformFeedbackBufferBase(tf, kTFAttribOutIndex, vbo);
    }
  }

  CHECK_GX_ERROR();
}

void MarchingCube::init_shaders() {
  // 1) build the density volume.
  {
    programs_.build_density = PROGRAM_ASSETS.createCompute(
      SHADERS_DIR "/marching_cube/01_density_volume/cs_buildDensityVolume.glsl"
    );

    int32_t const seed = 4567891 * (rand() / static_cast<float>(RAND_MAX)); //
    programs_.build_density->setUniform("uPerlinNoisePermutationSeed", seed);
  }

  // 2) list triangle to generate.
  {
    AssetId const asset_id = "MarchingCube::ListTriangle";

    programs_.trilist = PROGRAM_ASSETS.createGeo(
      asset_id,
      SHADERS_DIR "/marching_cube/02_list_triangle/vs_list_triangle.glsl",
      SHADERS_DIR "/marching_cube/02_list_triangle/gs_list_triangle.glsl"
    );

    auto const pgm = programs_.trilist->id;
    std::array<GLchar const*, 1> varyings{ "x6y6z6_e4e4e4" };

    glTransformFeedbackVaryings(pgm, varyings.size(), varyings.data(), GL_INTERLEAVED_ATTRIBS); //
    gx::LinkProgram(pgm);
    gx::CheckProgramStatus( pgm, asset_id ); //
  }

  // 3) generate the vertices buffers.
  {
    AssetId const asset_id = "MarchingCube::GenerateVertices";

    programs_.genvertices = PROGRAM_ASSETS.createGeo(
      asset_id,
      SHADERS_DIR "/marching_cube/03_generateVertices/vs_generateVertices.glsl",
      SHADERS_DIR "/marching_cube/03_generateVertices/gs_generateVertices.glsl"
    );

    auto const pgm = programs_.genvertices->id;
    std::array<GLchar const*, 2> varyings{ "outPositionWS", "outNormalWS" };

    glTransformFeedbackVaryings(pgm, varyings.size(), varyings.data(), GL_INTERLEAVED_ATTRIBS); //
    gx::LinkProgram(pgm);
    gx::CheckProgramStatus( pgm, asset_id );
  }

  // Rendering pass.
  {
    programs_.render_chunk = PROGRAM_ASSETS.createRender(
      "MarchingCube::Render",
      SHADERS_DIR "/unlit/vs_unlit.glsl",
      SHADERS_DIR "/unlit/fs_unlit.glsl"
    );
  }

  CHECK_GX_ERROR();
}

//-----------------------------------------------------------------------------

void MarchingCube::createChunk( glm::ivec3 const& coords ) {

  int const index = (grid_dim_.x * grid_dim_.y) * coords.z
                  + grid_dim_.x * coords.y 
                  + coords.x
                  ;
  auto &chunk = grid_[index];
  chunk.coords = coords;

  // Reset chunk states.
  if (chunk.id < 0) {
    glm::vec3 const start = - 0.5f * glm::vec3(grid_dim_);
    chunk.ws_coords = kChunkSize * (start + glm::vec3(chunk.coords));
    chunk.id = -1;
    chunk.state = CHUNK_EMPTY;
  }


  // 1) Generate the density volume.
  buildDensityVolume(chunk);

  gx::Enable( gx::State::RasterizerDiscard );
  {
    auto &query = trilist_.query;
    if (!query) { glCreateQueries(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1, &query); }

    // [better to use a compute shader]
    // 2) List triangles to output.
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
      listTriangles();
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    
    GLint query_result = 0;
    glGetQueryObjectiv( query, GL_QUERY_RESULT, &query_result);

    // 3) Generate the chunk's triangles (if any).
    bool const bHasTriangles = (query_result > 0);
    if (bHasTriangles) {
      generateVertices(chunk);
      chunk.state = CHUNK_FILLED;
    }
  }
  gx::Disable( gx::State::RasterizerDiscard );

  CHECK_GX_ERROR();
}

void MarchingCube::buildDensityVolume(ChunkInfo_t &chunk) {
  auto &pgm = programs_.build_density->id;

  float const t = GlobalClock::Get().applicationTime();
  gx::SetUniform( pgm, "uTime",       t);

  gx::SetUniform( pgm, "uInvChunkDim",     kInvChunkDim);
  gx::SetUniform( pgm, "uMargin",          static_cast<float>(kMargin));
  gx::SetUniform( pgm, "uChunkSizeWS",     kChunkSize);
  gx::SetUniform( pgm, "uChunkPositionWS", chunk.ws_coords);

  constexpr int32_t image_unit = 0;
  glBindImageTexture( image_unit, density_tex_->id, 0, GL_FALSE, 0, GL_WRITE_ONLY, density_tex_->internal_format()); //
  gx::SetUniform( pgm, "uDstImg", image_unit);

  gx::UseProgram(pgm);
    gx::DispatchCompute<4, 4, 4>( kTextureRes, kTextureRes, kTextureRes); //
  gx::UseProgram(0u);

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

  CHECK_GX_ERROR();
}

void MarchingCube::listTriangles() {
  auto const pgm = programs_.trilist->id;

  gx::SetUniform( pgm, "uMargin", static_cast<float>(kMargin));
  
  int32_t texunit = 0;
  {
    // note : TEXTURE_BUFFER & TEXTURE_3D can also use the same unit

    gx::BindTexture( density_tex_->id, texunit, gx::SamplerName::NearestClamp);
    gx::SetUniform( pgm, "uDensityVolume_nearest", texunit++);

    gx::BindTexture( tbo_.lut_tex, texunit, gx::SamplerName::NearestClamp);
    gx::SetUniform( pgm, "uCaseToNumTri", texunit++);

    gx::BindTexture( tbo_.edge_connect_tex, texunit, gx::SamplerName::NearestClamp);
    gx::SetUniform( pgm, "uEdgeConnectList", texunit++);
  }
  
  gx::UseProgram( pgm );
  glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, trilist_.tf );
  glBeginTransformFeedback( GL_POINTS );
  glBindVertexArray(trilist_.vao);
  {
    /// List triangles contains in every voxels.
    glDrawArraysInstanced(GL_POINTS, 0, kVoxelsPerSlice, kChunkDim); //
  }
  glBindVertexArray(0u);
  glEndTransformFeedback();
  glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
  gx::UseProgram( 0 );

  for (; texunit >= 0; --texunit) {
    gx::UnbindTexture(texunit);
  }

  CHECK_GX_ERROR();
}

void MarchingCube::generateVertices(ChunkInfo_t &chunk) {
  if ((chunk.id < 0) && (nfreebuffers_ <= 0)) {
    LOG_WARNING( "No more free space for chunk datas." );
    chunk.id = -1; //
    return;
  }

  if (chunk.id < 0) {
    chunk.id = freebuffer_indices_[kBufferBatchSize - nfreebuffers_];
    --nfreebuffers_;
  }

  auto const pgm = programs_.genvertices->id;
  gx::UseProgram(pgm);
  
  gx::SetUniform( pgm, "uChunkPositionWS",   chunk.ws_coords);
  gx::SetUniform( pgm, "uVoxelSize",         kVoxelSize); //
  gx::SetUniform( pgm, "uMargin",            float(kMargin)); //
  gx::SetUniform( pgm, "uInvWindowDim",      kInvWindowDim); //
  gx::SetUniform( pgm, "uWindowDim",         float(kWindowDim)); //

  {
    int32_t texunit = 0;

    gx::BindTexture( density_tex_->id, texunit, gx::SamplerName::NearestClamp);
    gx::SetUniform( pgm, "uDensityVolume_nearest", texunit++);

    gx::BindTexture( density_tex_->id, texunit, gx::SamplerName::LinearClamp);
    gx::SetUniform( pgm, "uDensityVolume_linear", texunit++);
  }

  auto const tf = tf_stack_[chunk.id];
  glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, tf );
  glBeginTransformFeedback( GL_POINTS );
  glBindVertexArray( genvertices_vao_ );
  {    
    glDrawTransformFeedback(GL_POINTS, trilist_.tf);
  }
  glBindVertexArray( 0u );
  glEndTransformFeedback();
  glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );

  gx::UseProgram( 0u );

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------