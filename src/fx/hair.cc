#include "fx/hair.h"
#include "shaders/hair/interop.h"

#include <array>
#include "glm/glm.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "im3d/im3d.h"

#include "core/camera.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/views/fx/HairView.h"

// ----------------------------------------------------------------------------

// @note : all strands share the same number of control points.
static constexpr int kNumControlPoints   = HAIR_MAX_PARTICLE_PER_STRAND;
static constexpr int kNumControlSegments = kNumControlPoints - 1;

static constexpr int kDebugRenderControlPointSize = 4;

// ----------------------------------------------------------------------------

void Hair::init() {
  // (This could be shared between hair instances)
  
  init_transform_feedbacks();
  init_shaders();

  // [should be capped by nfaces * maxtessLevel * maxInstances or smthg]
  randbuffer_.init( HAIR_TF_RANDOMBUFFER_SIZE );
  randbuffer_.generate_values();

  marschner_.init();
  marschner_.generate();

  init_ui_views();
}

void Hair::setup(ResourceId const& scalp_id) {
  auto scalp_resource{ Resources::Get<MeshData>(scalp_id) };

  if (!scalp_resource.is_valid()) {
    LOG_ERROR( "The scalp mesh resource was not found.\n", scalp_id.str() );
    return; 
  }

  auto &scalp_mesh_data{ *scalp_resource.data };

  glm::vec3 pivot{};
  glm::vec3 bounds{};
  float radius{};
  scalp_mesh_data.calculateBounds(pivot, bounds, radius); //
  //scalp_mesh_data.calculateBounds(&pivot, &bounds, &radius); // TODO

  nroots_ = scalp_mesh_data.nvertices();
  model_ = glm::translate( glm::mat4(1.0f), pivot);
  params_.readonly.nroots = nroots_;

  init_simulation( scalp_mesh_data );
  init_mesh( scalp_mesh_data );
}

void Hair::deinit() {
  // (buffers)
  pbuffer_.destroy();
  randbuffer_.deinit();

  // (mesh)
  if (mesh_.vao) {
    glDeleteVertexArrays(1u, &mesh_.vao);
    glDeleteBuffers(1u, &mesh_.ibo);
    mesh_.vao = 0u;
  }

  // (tf)
  if (tess_stream_.tf) {
    glDeleteTransformFeedbacks(1, &tess_stream_.tf);
    glDeleteBuffers(1, &tess_stream_.tf);
    glDeleteVertexArrays(1u, &tess_stream_.vao);
    tess_stream_.tf = 0;
  }
  
  nroots_ = 0;
}

void Hair::update(float const dt) {
  if (!initialized()) {
    LOG_DEBUG_INFO( "Calling Hair::update without initialization." );
    return;
  }

  // Update the reflectance model LUT when they've changed.
  marschner_.update();
  
  // [Debug : change scalp model matrix ]
  //Im3d::Gizmo( "HairScalp", glm::value_ptr(model_));

  // Simulation CS.
  pbuffer_.bind();
  {
    auto const pgm = pgm_.cs_simulation->id;

    gx::UseProgram(pgm);
      gx::SetUniform( pgm, "uTimeStep",       dt);
      gx::SetUniform( pgm, "uScaleFactor",    params_.render.lengthScale);
      gx::SetUniform( pgm, "uModel",          model_); 
      gx::SetUniform( pgm, "uBoundingSphere", boundingsphere_);

      gx::DispatchCompute(nroots_);
      //DispatchCompute<HAIR_MAX_PARTICLE_PER_STRAND>(pbuffer_.size());  //<=> glDispatchCompute(nroots_, 1u, 1u);
    gx::UseProgram();
  }
  pbuffer_.unbind();

  // Synchronize operations on buffers.
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

  // Copy back the write buffer to the read buffer.
  pbuffer_.swap();

  CHECK_GX_ERROR();
}

void Hair::render(Camera const& camera) {
  if (!initialized()) {
    LOG_DEBUG_INFO( "Calling Hair::render without initialization." );
    return;
  }

  // Note :
  // The first and second passes could be merged in a single one if we only
  // need to render the hair once.
  // As the tesselator cannot output lines_adjacency informations for the following
  // geometry stage, we need two passes at least to fix the triangle strip generation
  // (only noticeable at close range though).
  // [@todo compare performances between 1 and 2 passes rendering.]

  if (!params_.render.bShowDebugCP)
  {
    gx::Enable( gx::State::RasterizerDiscard );

    // 1) Stream tesselated hairs to buffer [put in update stage ?].
    {
      auto const pgm = pgm_.tess_stream->id;

      gx::UseProgram(pgm);
        randbuffer_.bind(SSBO_HAIR_TF_RANDOMBUFFER);

        // (TESS)
        gx::SetUniform( pgm, "uNumInstances",    params_.tess.ninstances);
        gx::SetUniform( pgm, "uNumLines",        params_.tess.nlines);
        gx::SetUniform( pgm, "uNumSubSegments",  params_.tess.nsubsegments);
        gx::SetUniform( pgm, "uScaleFactor",     params_.render.lengthScale);

        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tess_stream_.tf);
        glBeginTransformFeedback(GL_LINES);

        glBindVertexArray(mesh_.vao);
          glPatchParameteri(GL_PATCH_VERTICES, mesh_.patchsize);
          glDrawElementsInstanced(GL_PATCHES, mesh_.nelems, GL_UNSIGNED_INT, nullptr, params_.tess.ninstances);
        glBindVertexArray(0u);

        glEndTransformFeedback();
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

        randbuffer_.unbind(SSBO_HAIR_TF_RANDOMBUFFER);
      gx::UseProgram(0);
    }

    gx::Disable( gx::State::RasterizerDiscard );

    // 2) Render hair as camera aligned ribbons.
    {
      auto const pgm = pgm_.render->id;

      gx::UseProgram(pgm);
        // (VS)
        gx::SetUniform( pgm, "uMVP",              camera.viewproj());
        // (GS)
        gx::SetUniform( pgm, "uView",             camera.view());
        gx::SetUniform( pgm, "uProjection",       camera.proj());
        gx::SetUniform( pgm, "uLineWidth",        params_.render.linewidth);
        // (FS)
        gx::SetUniform( pgm, "uLongitudinalLUT",  0);
        gx::SetUniform( pgm, "uAzimuthalLUT",     1);
        gx::SetUniform( pgm, "uAlbedo",           params_.render.albedo);

        marschner_.bindLUTs();

        glBindVertexArray(tess_stream_.vao);
          glDrawTransformFeedback(GL_LINES, tess_stream_.tf); //
        glBindVertexArray(0);

        marschner_.unbindLUTs();

      gx::UseProgram(0);
    }
  }

  // --------------------------------

  // Simple debug rendering (show control points).
  if (params_.render.bShowDebugCP) {
    auto const& pgm = pgm_.render_debug->id;

    glm::vec4 const color(1.0f, 0.0f, 0.0f, 0.9f);

    gx::UseProgram(pgm);
      gx::SetUniform( pgm, "uMVP",    camera.viewproj());
      gx::SetUniform( pgm, "uColor",  color);

      glPointSize(kDebugRenderControlPointSize);

      glBindVertexArray(mesh_.vao);
        glDrawArrays( GL_POINTS, 0, pbuffer_.size());
      glBindVertexArray(0);

      glPointSize(1);
    gx::UseProgram();
  }

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void Hair::init_ui_views() {
  params_.readonly.nControlPoints = kNumControlPoints;
  params_.ui_marschner = marschner_.ui_view; //
  ui_view = std::make_shared<views::HairView>(params_);
}

void Hair::init_simulation(MeshData const& scalpMesh) {
  // Host attribs buffers to initialize the particles (ie. strands control points).
  using AttribBuffer_t = std::vector<glm::vec4>;
  std::array<AttribBuffer_t, NUM_SSBO_HAIR_SIM_ATTRIBS> host_attribs_;

  // Host buffer references for easier access.
  auto& Positions  = host_attribs_[pbuffer_.attrib_index(SSBO_HAIR_SIM_POSITION_READ)];
  auto& Velocities = host_attribs_[pbuffer_.attrib_index(SSBO_HAIR_SIM_VELOCITY_READ)];
  auto& Tangents   = host_attribs_[pbuffer_.attrib_index(SSBO_HAIR_SIM_TANGENT_READ)];  //

  // Resize buffers.
  int const npoints = nroots_ * kNumControlPoints;
  for (auto &buffer : host_attribs_) {
    buffer.resize(npoints);
  }

  // Initialize base normals used for offsetting strands from the mesh.
  // (base normals are kept for potential future uses)
  normals_.resize(npoints);
  for (int j = 0; j < nroots_; ++j) {
    auto const& v = scalpMesh.vertices[j];
    int const rootVertexIndex = j * kNumControlPoints;
    Positions[rootVertexIndex] = glm::vec4(v.position, 0);
    normals_[rootVertexIndex] = v.normal;
  }

  // -------------

  // -- Setup initial particles Positions & Velocities.
  float const scaleOffset = params_.sim.maxlength / static_cast<float>(kNumControlPoints);
  for (int j = 0; j < nroots_; ++j) {
    int const rootVertexIndex = j * kNumControlPoints;

    auto const& v = glm::vec3(Positions[rootVertexIndex]);
    auto const& n = normals_[rootVertexIndex];

    // (dirty 'organic' size randomness scaling)
    float const random_value{static_cast<float>( 
      1.0 + 0.1 * (1.0 - 2.0 * static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) //
    )};

    float lastOffset = 0.0f;
    for (int i = 0u; i < kNumControlPoints; ++i) {
      int const idx{ rootVertexIndex + i };
      // Segment length grow exponentially. [! makes the system less stable]
      float const offset{ 
        static_cast<float>(i) * scaleOffset * random_value
      };
      Positions[idx]  = glm::vec4(v + offset * n, offset - lastOffset);
      Velocities[idx] = glm::vec4(0.0);
      lastOffset      = offset;
    }
  }

  // -- Setup Tangents. [to be removed]
  {
    float const inv_nroots = 1.0f / static_cast<float>(nroots_);
    float const kPi      = glm::pi<float>();
    float const scaleMaxLength = 0.125f * sqrtf(params_.sim.maxlength);
    
    bool const bCurly = true;
    glm::vec3 curly(0.0f);

    for (int j = 0u; j < nroots_; ++j) {
      int const A = j * kNumControlPoints;
      int const B = A + (kNumControlPoints - 1u);
      float const dj = static_cast<float>(j + 1) * inv_nroots;

      if (bCurly) {
        float n = 1.25f * glm::simplex( glm::vec2(sinf(3.0f * dj), cosf(5.0f)) );
        curly = glm::vec3(cosf(n * 4.0f * kPi), -0.71f * n, sinf(n * 2.7f * kPi));
      }

      // Outer tangent.
      Tangents[A] = .15f*glm::vec4(normals_[A], 0);
      Tangents[B] = .2f*glm::vec4(-normals_[A] + curly, 0);

      // Inner tangents.
      float const dist_AB = static_cast<float>(B - A);
      float const inv_dist = 1.0f / dist_AB;
      for (int i = A + 1u; i < B; ++i) {
      
        if (bCurly) {  
          float const di = 10.0f * static_cast<float>(B - i) / (dist_AB - 1.0f);
          float const n = di * glm::simplex( glm::vec2(sinf(43.0f * dj), cosf(5.0f * di)) );
          curly = -5.8f * glm::vec3(10.7f * cosf(n * kPi), -2.3f * n, 20.5f * sinf(n * kPi));
        }

        float const s = 0.1f * static_cast<float>(i - A) * inv_dist * scaleMaxLength;
        Tangents[i] = s * glm::vec4(curly, 0);
      }
    }
  }

  // -------------

  // Create the device buffer.
  pbuffer_.setup(npoints, SSBO_HAIR_SIM_FIRST_BINDING, NUM_SSBO_HAIR_SIM_ATTRIBS);
  
  // Upload the data from host to device.
  auto const buffer_id = pbuffer_.read_ssbo_id();
  auto const attrib_bytesize = pbuffer_.attrib_buffer_bytesize();
  for (int i = 0; i < NUM_SSBO_HAIR_SIM_ATTRIBS; ++i) {
    glNamedBufferSubData(buffer_id, i*attrib_bytesize, attrib_bytesize, host_attribs_[i].data());
  }

  glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT );

  // [TMP] ----------------------------------------------------
  // As tangents are not used in the simulation CS pass, we need to move them
  // once to the WRITE buffer.
  // (before the whole buffer get copied back to the READ buffer).
  auto const tang_id = pbuffer_.attrib_index(SSBO_HAIR_SIM_TANGENT_READ);

  glCopyNamedBufferSubData(
    pbuffer_.read_ssbo_id(), 
    pbuffer_.write_ssbo_id(), 
    tang_id * attrib_bytesize, 
    tang_id * attrib_bytesize, 
    attrib_bytesize
  );
  glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT );
  // ----------------------------------------------------

  CHECK_GX_ERROR();
}

void Hair::init_mesh(MeshData const& scalpMesh) {
  /// Here we define the VAO used by the tesselation pass.
  /// It binds the position & tangents part of the PingPong buffer
  /// and create an element buffer for isolines tesselation patches.

  glCreateVertexArrays(1, &mesh_.vao);
 
  // -- Attributes.
  auto const vbo = pbuffer_.read_ssbo_id();
  std::array<uint32_t[2], 2> const buffer_attrib_bindings{
    vbo, SSBO_HAIR_SIM_POSITION_READ,
    vbo, SSBO_HAIR_SIM_TANGENT_READ,
  };
  
  glBindVertexArray(mesh_.vao);
  {
    int binding = -1;
    for (auto const& vbo_binding : buffer_attrib_bindings) {
      auto const buffer      = vbo_binding[0];
      auto const attrib_bind = vbo_binding[1];
      auto const offset      = pbuffer_.attrib_index(attrib_bind) * pbuffer_.attrib_buffer_bytesize();

      glBindVertexBuffer(++binding, buffer, offset, PingPongBuffer::kAttribBytesize);
      glVertexAttribFormat(attrib_bind, 4u, GL_FLOAT, GL_FALSE, 0);
      glVertexAttribBinding(attrib_bind, binding);
      glEnableVertexAttribArray(attrib_bind);
    }
  }
  glBindVertexArray(0);

  // -- Elements.
  // Setup elements for isoline patches.
  // for each triangles we use 2 control points per strands subsegments 
  // (start and end position of the segment)
  mesh_.patchsize = 6;
  mesh_.nelems    = mesh_.patchsize * scalpMesh.nfaces() * kNumControlSegments;
  std::vector<int> elements(mesh_.nelems);
  int idx = 0;
  for (int i = 0; i < scalpMesh.nfaces(); ++i) {
    for (int j = 0; j < kNumControlSegments; ++j) {
      for (int k = 0; k < 3; ++k) {
        int const e = kNumControlPoints * scalpMesh.indices[3*i + k] + j;
        elements[idx++] = e;
        elements[idx++] = e + 1; 
      }
    }
  }

  glCreateBuffers(1, &mesh_.ibo);
  auto const bytesize = static_cast<uint32_t>(elements.size() * sizeof(elements[0]));
  glNamedBufferStorage(mesh_.ibo, bytesize, elements.data(), 0);
  glVertexArrayElementBuffer(mesh_.vao, mesh_.ibo);

  CHECK_GX_ERROR();
}

void Hair::init_transform_feedbacks() {
  auto &buffer = tess_stream_.strands_buffer_id;
  auto &tf = tess_stream_.tf;
  auto &vao = tess_stream_.vao;

  // The real value should be around : 
  //    maxTessLevel0 * maxTessLevel1 * maxInstance * (nroots_ * kNumControlPoints) * sizeof(float3)
  // !! We should check that the buffer could handle it.
  auto const kStreamBufferBytesize{ 64 * 1024 * 1024 };  //  [xxx bug prone]

  // 1) Create the buffer to hold the raw tesselated strands.
  glCreateBuffers( 1u, &buffer);
  glNamedBufferStorage(buffer, kStreamBufferBytesize, nullptr, 0u);

  // 2) Create the transform feedback used to fill it, and bind the buffer with it.
  glCreateTransformFeedbacks(1, &tf);
  glTransformFeedbackBufferBase(tf, BINDING_HAIR_TF_ATTRIB_OUT, buffer);

  // 3) Create the VAO used to render its content.
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);
    glBindVertexBuffer(0, buffer, 0, 4*sizeof(float));
    glVertexAttribFormat(BINDING_HAIR_RENDER_ATTRIB_IN, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribBinding(BINDING_HAIR_RENDER_ATTRIB_IN, 0);
    glEnableVertexAttribArray(BINDING_HAIR_RENDER_ATTRIB_IN);
  glBindVertexArray(0);

  CHECK_GX_ERROR();
}

void Hair::init_shaders() {
  // Simulation pass.
  pgm_.cs_simulation = PROGRAM_ASSETS.createCompute(
    SHADERS_DIR "/hair/01_simulation/cs_simulation.glsl"
  );

  // Tesselation stream pass.
  {
    AssetId assetId( "hair::tessFeedback" );
    ProgramParameters params{
      SHADERS_DIR "/hair/02_tess_stream/vs_stream_hair.glsl",
      SHADERS_DIR "/hair/02_tess_stream/tcs_stream_hair.glsl",
      SHADERS_DIR "/hair/02_tess_stream/tes_stream_hair.glsl",
      SHADERS_DIR "/hair/02_tess_stream/gs_stream_hair.glsl"
    };

    pgm_.tess_stream = PROGRAM_ASSETS.create( assetId, params );

    // [ to wrap ]
    {
      auto const pgm = pgm_.tess_stream->id;

      // Set transform feedback outputs.
      std::array<GLchar const*, 1> varyings{ "position_xyz_coeff_w" };
      glTransformFeedbackVaryings(pgm, varyings.size(), varyings.data(), GL_INTERLEAVED_ATTRIBS); //

      // (linked manually because of TF)
      gx::LinkProgram(pgm);
      gx::CheckProgramStatus( pgm, assetId );
    }
  }

  // Geometry / Render pass.
  pgm_.render = PROGRAM_ASSETS.createGeo(
    "hair::geo",
    SHADERS_DIR "/hair/03_rendering/vs_render_hair.glsl",
    SHADERS_DIR "/hair/03_rendering/gs_render_hair.glsl",
    SHADERS_DIR "/hair/03_rendering/fs_render_hair.glsl"
  );

  // Debug render.
  pgm_.render_debug = PROGRAM_ASSETS.createRender(
    "hair::debug",
    SHADERS_DIR "/unlit/vs_unlit.glsl",
    SHADERS_DIR "/unlit/fs_unlit.glsl"
  );

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------