#include "fx/gpu_particle.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "ui/views/fx/SparkleView.h"
#include "shaders/particle/interop.h"
#include "core/logger.h"
#include "memory/assets/assets.h"

// ----------------------------------------------------------------------------

uint32_t constexpr GPUParticle::kThreadsGroupWidth = PARTICLES_KERNEL_GROUP_WIDTH;
uint32_t constexpr GPUParticle::kBatchEmitCount;

// ----------------------------------------------------------------------------

namespace {

// Layout of the indirect buffer (Dispatch + Draw)
struct TIndirectValues {
  uint32_t dispatch_x;
  uint32_t dispatch_y;
  uint32_t dispatch_z;
  uint32_t draw_count;
  uint32_t draw_primCount;
  uint32_t draw_first;
  uint32_t draw_reserved;
};

uint32_t GetClosestPowerOfTwo(uint32_t const n) {
  uint32_t r = 1u;
  for (uint32_t i = 0u; r < n; r <<= 1u) ++i;
  return r;
}

uint32_t GetNumTrailingBits(uint32_t const n) {
  uint32_t r = 0u;
  for (uint32_t i = n; i > 1u; i >>= 1u) ++r;
  return r;
}

void SwapUint(uint32_t &a, uint32_t &b) {
  a ^= b;
  b ^= a;
  a ^= b;
}

}  // namespace

// ----------------------------------------------------------------------------

void GPUParticle::init() {
  // Assert than the number of particles will be a factor of threadGroupWidth.
  auto const nparticles = FloorParticleCount(kMaxParticleCount);
  auto const nattribs   = PingPongBuffer::NumAttribsRequired<TParticle>();
  //LOG_INFO( "[", nparticles, "particles,", kBatchEmitCount, "per batch ]" );

  // Particle attributes double buffer.
  pbuffer_.setup(nparticles, 0, nattribs, SPARKLE_USE_SOA_LAYOUT);

  // Setup VAO for rendering.  
  init_vao();

  // Simulation passes buffers.
  init_buffers();

  // Random values StorageBuffer.
  randbuffer_.init(3u * nparticles);

  gx::Enable( gx::State::ProgramPointSize );
  init_shaders();

  // Query used for benchmarking.
  glCreateQueries(GL_TIME_ELAPSED, 1, &query_time_);

  init_ui_views();

  CHECK_GX_ERROR();
}

void GPUParticle::deinit() {
  randbuffer_.deinit();

  glDeleteQueries(1, &query_time_);

  glDeleteBuffers(2u, gl_atomic_buffer_ids_.data());
  glDeleteBuffers(1u, &gl_indirect_buffer_id_);
  glDeleteBuffers(1u, &gl_dp_buffer_id_);
  glDeleteBuffers(1u, &gl_sort_indices_buffer_id_);

  glDeleteVertexArrays(1u, &vao_);

  pbuffer_.destroy();
}

void GPUParticle::update(float const dt, Camera const& camera) {

  // Max number of particles able to be spawned. 
  uint32_t const num_dead_particles = pbuffer_.size() - num_alive_particles_;

  // Number of particles to be emitted. 
  uint32_t const emit_count = std::min(kBatchEmitCount, num_dead_particles); //

  // Simulation deltatime depends on application framerate and the user input.
  float const time_step = dt * params_.simulation.time_step_factor;

  // Update random buffer with new values.
  randbuffer_.generate_values();

  pbuffer_.bind();
  {
    glBindBuffersBase(GL_ATOMIC_COUNTER_BUFFER, ATOMIC_COUNTER_BINDING_FIRST, 2u, gl_atomic_buffer_ids_.data());
    randbuffer_.bind(STORAGE_BINDING_RANDOM_VALUES);
    {
      // Emission stage : write in buffer A.
      _emission(emit_count);

      // Simulation stage : read buffer A, write buffer B.
      _simulation(time_step);
    }
    randbuffer_.unbind(STORAGE_BINDING_RANDOM_VALUES);
    glBindBuffersBase(GL_ATOMIC_COUNTER_BUFFER, ATOMIC_COUNTER_BINDING_FIRST, 2u, nullptr);

    // Sort particles for alpha-blending. 
    if (enable_sorting_ && simulated_) {
      _sorting(camera.view());
    }
  }
  pbuffer_.unbind();

  // PostProcess stage.
  _postprocess();

  CHECK_GX_ERROR();
}

void GPUParticle::render(Camera const& camera) {
  auto const& params = params_.rendering;

  // [ uniform buffers would be simpler here ]

  switch(params.rendermode) {
    case RENDERMODE_STRETCHED:
    {
      auto &pgm = pgm_.render_stretched_sprite->id;

      gx::UseProgram( pgm );
      gx::SetUniform( pgm, "uView",                 camera.view());
      gx::SetUniform( pgm, "uMVP",                  camera.viewproj());
      gx::SetUniform( pgm, "uColorMode",            uint32_t(params.colormode));
      gx::SetUniform( pgm, "uBirthGradient",        params.birth_gradient);
      gx::SetUniform( pgm, "uDeathGradient",        params.death_gradient);
      gx::SetUniform( pgm, "uSpriteStretchFactor",  params.stretched_factor);
      gx::SetUniform( pgm, "uFadeCoefficient",      params.fading_factor);
    }
    break;

    case RENDERMODE_POINTSPRITE:
    default:
    {
      auto &pgm = pgm_.render_point_sprite->id;
      
      gx::UseProgram( pgm );
      gx::SetUniform( pgm, "uMVP",              camera.viewproj());
      gx::SetUniform( pgm, "uMinParticleSize",  params.min_size);
      gx::SetUniform( pgm, "uMaxParticleSize",  params.max_size);
      gx::SetUniform( pgm, "uColorMode",        uint32_t(params.colormode));
      gx::SetUniform( pgm, "uBirthGradient",    params.birth_gradient);
      gx::SetUniform( pgm, "uDeathGradient",    params.death_gradient);
      gx::SetUniform( pgm, "uFadeCoefficient",  params.fading_factor);
    }
    break;
  }

  glBindVertexArray(vao_);
    auto const offset = reinterpret_cast<void const*>(offsetof(TIndirectValues, draw_count));
    
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gl_indirect_buffer_id_);
    glDrawArraysIndirect(GL_POINTS, offset);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0u);
  glBindVertexArray(0u);
  gx::UseProgram();

  CHECK_GX_ERROR();
}

void GPUParticle::render_debug_particles(Camera const& camera) {
  auto const& params = simulation_parameters(); //
  float const radius = params.emitter_radius;

  glm::vec3 scale(1.0f);

  switch (params.emitter_type) {
    case GPUParticle::EMITTER_DISK:
      scale = glm::vec3(radius, 0.0f, radius);
    break;

    case GPUParticle::EMITTER_SPHERE:
    case GPUParticle::EMITTER_BALL:
      scale = glm::vec3(radius);
    break;

    case GPUParticle::EMITTER_POINT:
    default:
    break;
  }

  gx::Disable( gx::State::CullFace );
  gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Line);

#if 0
  gx::UseProgram( pgm_handle_->id );
  {
    // emitter.
    glm::mat4 model = glm::translate(glm::mat4(1.0f), params.emitter_position)
                    * glm::scale(glm::mat4(1.0f), scale);
    glm::vec4 color = glm::vec4(0.9f, 0.9f, 1.0f, 0.5f);
    draw_mesh( debug_sphere_, model, color, camera);

    // bounding volume.
    model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f*params.bounding_volume_size));
    color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
    draw_mesh( debug_sphere_, model, color, camera);
  }
  gx::UseProgram();
#endif 
  
  gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Fill);
  gx::Enable( gx::State::CullFace );
}

// ----------------------------------------------------------------------------

void GPUParticle::init_vao() {
  glCreateVertexArrays(1u, &vao_);
  glBindVertexArray(vao_);

  auto const vbo = pbuffer_.read_ssbo_id();

  if constexpr(SPARKLE_USE_SOA_LAYOUT) {

    auto const attrib_size = PingPongBuffer::kAttribBytesize; // vec4
    auto const attrib_buffer_size = pbuffer_.attrib_buffer_bytesize();

    GLuint binding_point = 0u;
    GLuint attrib_index = 0u;

    // Position.
    binding_point = STORAGE_BINDING_PARTICLE_POSITIONS_A;
    glBindVertexBuffer(binding_point, vbo, attrib_index*attrib_buffer_size, attrib_size);
    {
      uint32_t const num_component = 3u;
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, 0);
      glVertexAttribBinding(attrib_index, binding_point);
      glEnableVertexAttribArray(attrib_index);
      ++attrib_index;
    }

    // Velocity.
    binding_point = STORAGE_BINDING_PARTICLE_VELOCITIES_A;
    glBindVertexBuffer(binding_point, vbo, attrib_index*attrib_buffer_size, attrib_size);
    {
      uint32_t const num_component = 3u;
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, 0);
      glVertexAttribBinding(attrib_index, binding_point);
      glEnableVertexAttribArray(attrib_index);
      ++attrib_index;
    }

    // Age attributes.
    binding_point = STORAGE_BINDING_PARTICLE_ATTRIBUTES_A;
    glBindVertexBuffer(binding_point, vbo, attrib_index*attrib_buffer_size, attrib_size);
    {
      uint32_t const num_component = 2u;
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, 0);
      glVertexAttribBinding(attrib_index, binding_point);
      glEnableVertexAttribArray(attrib_index);
      ++attrib_index;
    }
  } else {
    uint32_t const binding_index = 0u;
    glBindVertexBuffer(binding_index, vbo, 0u, sizeof(TParticle));
    // Positions.
    {
      uint32_t const attrib_index = 0u;
      uint32_t const num_component = static_cast<uint32_t>((sizeof TParticle::position) / sizeof(TParticle::position[0u]));
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, offsetof(TParticle, position));
      glVertexAttribBinding(attrib_index, binding_index);
      glEnableVertexAttribArray(attrib_index);
    }
    // Velocities.
    {
      uint32_t const attrib_index = 1u;
      uint32_t const num_component = static_cast<uint32_t>((sizeof TParticle::velocity) / sizeof(TParticle::velocity[0u]));
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, offsetof(TParticle, velocity));
      glVertexAttribBinding(attrib_index, binding_index);
      glEnableVertexAttribArray(attrib_index);
    }
    // Age attributes.
    {
      uint32_t const attrib_index = 2u;
      uint32_t const num_component = 2u;
      glVertexAttribFormat(attrib_index, num_component, GL_FLOAT, GL_FALSE, offsetof(TParticle, start_age)); //
      glVertexAttribBinding(attrib_index, binding_index);
      glEnableVertexAttribArray(attrib_index);
    }
  }

  glBindVertexArray(0u);

  CHECK_GX_ERROR();
}

void GPUParticle::init_buffers() {
  // Atomic Counter buffers.
  {
    GLuint const default_values[2u]{ 0u, 0u };
    glCreateBuffers(2u, gl_atomic_buffer_ids_.data());

    glNamedBufferStorage(gl_atomic_buffer_ids_[0u], sizeof default_values[0u], &default_values[0u],
      GL_MAP_READ_BIT
    );
    glNamedBufferStorage(gl_atomic_buffer_ids_[1u], sizeof default_values[1u], &default_values[1u],
      GL_MAP_READ_BIT
    );
  }

  // Dispatch and Draw Indirect buffer.
  {
    TIndirectValues constexpr default_indirect[]{
      1u, 1u, 1u,     // Dispatch values
      0, 1u, 0u, 0u   // Draw values
    };
    glCreateBuffers(1u, &gl_indirect_buffer_id_);
    glNamedBufferStorage(gl_indirect_buffer_id_, sizeof default_indirect, default_indirect, 0);
  }

  // Sorting StorageBuffers.
  {
    // The parallel nature of the sorting algorithm needs power of two sized buffer.
    auto const sort_buffer_max_count = GetClosestPowerOfTwo(kMaxParticleCount); //

    // DotProducts buffer.
    GLuint const dp_buffer_size = sort_buffer_max_count * sizeof(GLfloat);
    glCreateBuffers(1u, &gl_dp_buffer_id_);
    glNamedBufferStorage( gl_dp_buffer_id_, dp_buffer_size, nullptr, 0);

    // Double-sized buffer for indices sorting.
    // [we might want to use u16 instead, when below ~64K particles]
    GLuint const sort_indices_buffer_size = 2u * sort_buffer_max_count * sizeof(GLuint);
    glCreateBuffers(1u, &gl_sort_indices_buffer_id_);
    glNamedBufferStorage(gl_sort_indices_buffer_id_, sort_indices_buffer_size, nullptr, 0);
  }

  CHECK_GX_ERROR();
}

void GPUParticle::init_shaders() {
  pgm_.emission     = PROGRAM_ASSETS.createCompute(SHADERS_DIR "/particle/01_emission/cs_emission.glsl" );
  pgm_.update_args  = PROGRAM_ASSETS.createCompute(SHADERS_DIR "/particle/02_simulation/cs_update_args.glsl" );
  pgm_.simulation   = PROGRAM_ASSETS.createCompute(SHADERS_DIR "/particle/02_simulation/cs_simulation.glsl" );
  pgm_.fill_indices = PROGRAM_ASSETS.createCompute(SHADERS_DIR "/particle/03_sorting/cs_fill_indices.glsl" );
  pgm_.calculate_dp = PROGRAM_ASSETS.createCompute(SHADERS_DIR "/particle/03_sorting/cs_calculate_dp.glsl" );
  pgm_.sort_step    = PROGRAM_ASSETS.createCompute(SHADERS_DIR "/particle/03_sorting/cs_sort_step.glsl" );
  pgm_.sort_final   = PROGRAM_ASSETS.createCompute(SHADERS_DIR "/particle/03_sorting/cs_sort_final.glsl" );

  pgm_.render_point_sprite = PROGRAM_ASSETS.createRender( 
    "sparkle::PointSprite",
    SHADERS_DIR "/particle/04_rendering/vs_generic.glsl",
    SHADERS_DIR "/particle/04_rendering/fs_point_sprite.glsl"
  );

  pgm_.render_stretched_sprite = PROGRAM_ASSETS.createGeo(
    "sparkle::StretchedSprite",
    SHADERS_DIR "/particle/04_rendering/vs_generic.glsl",
    SHADERS_DIR "/particle/04_rendering/gs_stretched_sprite.glsl",
    SHADERS_DIR "/particle/04_rendering/fs_stretched_sprite.glsl"
  );

  // One time uniform setting.
  pgm_.simulation->setUniform( "uPerlinNoisePermutationSeed", rand()); // (reset after relinking ?)

  CHECK_GX_ERROR();
}

void GPUParticle::init_ui_views() {
  ui_view = std::make_shared<views::SparkleView>(params_);
}

void GPUParticle::_emission(uint32_t const count) {
  if (count <= 0) {
    return;
  }

  // Emit only if a minimum count is reached. 
  // if (count < kBatchEmitCount) {
  //   return;
  // }

  auto &pgm = pgm_.emission->id;
  gx::UseProgram( pgm );
  {
    auto const& params = params_.simulation;

    gx::SetUniform( pgm, "uEmitCount",            count);
    gx::SetUniform( pgm, "uEmitterType",          uint32_t(params.emitter_type));
    gx::SetUniform( pgm, "uEmitterPosition",      params.emitter_position);
    gx::SetUniform( pgm, "uEmitterDirection",     params.emitter_direction);
    gx::SetUniform( pgm, "uEmitterRadius",        params.emitter_radius);
    gx::SetUniform( pgm, "uParticleMinAge",       params.min_age);
    gx::SetUniform( pgm, "uParticleMaxAge",       params.max_age); 
    gx::DispatchCompute<kThreadsGroupWidth>(count);
  }
  gx::UseProgram();

  glMemoryBarrier( GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT );

  // Number of particles expected to be simulated. 
  num_alive_particles_ += count;

  CHECK_GX_ERROR();
}

void GPUParticle::_simulation(float const time_step) {
  if (num_alive_particles_ <= 0u) {
    simulated_ = false;
    return;
  }

  // Update Indirect arguments buffer for simulation dispatch and draw indirect. 
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDIRECT_ARGS, gl_indirect_buffer_id_);
  gx::UseProgram(pgm_.update_args->id);
    gx::DispatchCompute();
  gx::UseProgram(0u);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDIRECT_ARGS, 0u);

  // Synchronize the indirect argument buffer.
  glMemoryBarrier( GL_COMMAND_BARRIER_BIT );

  // Simulation Kernel.

  // if (enable_vectorfield_) {
  //   glBindTexture(GL_TEXTURE_3D, vectorfield_.texture_id());
  // }

  auto &pgm = pgm_.simulation->id;

  gx::UseProgram( pgm );
  {
    auto const& params = params_.simulation;
     
    gx::SetUniform( pgm, "uTimeStep",           time_step);
    gx::SetUniform( pgm, "uVectorFieldSampler", 0);
    gx::SetUniform( pgm, "uBoundingVolume",     int32_t(params.bounding_volume));
    gx::SetUniform( pgm, "uBBoxSize",           params.bounding_volume_size);

    gx::SetUniform( pgm, "uScatteringFactor",   params.scattering_factor);
    gx::SetUniform( pgm, "uVectorFieldFactor",  params.vectorfield_factor);
    gx::SetUniform( pgm, "uCurlNoiseFactor",    params.curlnoise_factor);
    gx::SetUniform( pgm, "uCurlNoiseScale",     1.0f / params.curlnoise_scale);
    gx::SetUniform( pgm, "uVelocityFactor",     params.velocity_factor);

    gx::SetUniform( pgm, "uEnableScattering",   params.enable_scattering);
    gx::SetUniform( pgm, "uEnableVectorField",  params.enable_vectorfield);
    gx::SetUniform( pgm, "uEnableCurlNoise",    params.enable_curlnoise);
    gx::SetUniform( pgm, "uEnableVelocityControl", params.enable_velocity_control);

    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, gl_indirect_buffer_id_);
      glDispatchComputeIndirect(0);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0u);
  }
  gx::UseProgram(0u);

  //glBindTexture(GL_TEXTURE_3D, 0u);

  // Synchronize operations on buffers. 
  glMemoryBarrier(
      GL_ATOMIC_COUNTER_BARRIER_BIT 
    | GL_SHADER_STORAGE_BARRIER_BIT 
    | GL_BUFFER_UPDATE_BARRIER_BIT
  );

  // Retrieve the number of alive particles to be used in the next frame. 
  /// @note Needed if we want to emit new particles.
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, gl_atomic_buffer_ids_[1u]);
  {
    /// @warning most costly call.
    num_alive_particles_ = *reinterpret_cast<GLuint*>(glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY));
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
  }
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0u);
  CHECK_GX_ERROR();

  simulated_ = true;
}

void GPUParticle::_sorting(glm::mat4 const& view) {
  // [ there is probably some remaining issues on kernels boundaries ]

  // The algorithm works on buffer sized in power of two. 
  auto const max_elem_count = GetClosestPowerOfTwo(num_alive_particles_);

  /// 1) Initialize indices and dotproducts buffers.
  { 
    // a) Fill first part of the indices buffer with continuous indices.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDICES_FIRST, gl_sort_indices_buffer_id_);
    gx::UseProgram(pgm_.fill_indices->id);
      gx::DispatchCompute<kThreadsGroupWidth>(max_elem_count);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDICES_FIRST, 0u);

    // b) Clear the dot product buffer.
    float const clear_value = -FLT_MAX;
    glClearNamedBufferSubData(
      gl_dp_buffer_id_, GL_R32F, 0u, max_elem_count * sizeof(float), GL_RED, GL_FLOAT, &clear_value
    );

    // c) Compute dot products between particles and the camera direction.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_DOT_PRODUCTS, gl_dp_buffer_id_);
    gx::UseProgram(pgm_.calculate_dp->id);
    {
      pgm_.calculate_dp->setUniform("uViewMatrix", view);

      // [No kernel boundaries check are performed]
      gx::DispatchCompute<kThreadsGroupWidth>(num_alive_particles_);
    }

    // Synchronize the indices and dotproducts buffers. 
    glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
  }
  // -----------------
  
  auto const indices_half_size = max_elem_count * sizeof(GLuint);
  uint32_t binding = 0u;

  /// 2) Sort particle indices through their dot products. 
  // [might be able to optimize early steps with one kernel, when max_block_width <= kernel_size]
  {
    auto const nthreads = max_elem_count / 2u;
    auto const nsteps = GetNumTrailingBits(max_elem_count);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_DOT_PRODUCTS, gl_dp_buffer_id_);
    
    auto const pgm = pgm_.sort_step->id;
    gx::UseProgram(pgm);

    for (uint32_t step = 0u; step < nsteps; ++step) {
      for (uint32_t stage = 0u; stage < step + 1u; ++stage) {
        // bind read / write indices buffers.
        auto const offset_read  = indices_half_size * binding;
        auto const offset_write = indices_half_size * (binding ^ 1u);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDICES_FIRST,  gl_sort_indices_buffer_id_,  offset_read, indices_half_size);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDICES_SECOND, gl_sort_indices_buffer_id_, offset_write, indices_half_size);
        binding ^= 1u;

        // compute kernel parameters.
        uint32_t const block_width     = 2u << (step - stage);
        uint32_t const max_block_width = 2u << step;

        gx::SetUniform( pgm, "uBlockWidth",    block_width);
        gx::SetUniform( pgm, "uMaxBlockWidth", max_block_width);
        gx::DispatchCompute<kThreadsGroupWidth>(nthreads);

        glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
      }
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_DOT_PRODUCTS, 0u);
  }

  // 3) Sort particles datas with their sorted indices.
  { 
    // bind the sorted indices to the first binding slot.
    auto const sorted_offset_read = indices_half_size * binding;
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDICES_FIRST, gl_sort_indices_buffer_id_, sorted_offset_read, indices_half_size);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDICES_SECOND, 0u);
    gx::UseProgram(pgm_.sort_final->id);
    {
      // [ could use the DispatchIndirect buffer ]
      gx::DispatchCompute<kThreadsGroupWidth>(num_alive_particles_);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, STORAGE_BINDING_INDICES_FIRST, 0u);
  }
  gx::UseProgram(0u);

  CHECK_GX_ERROR();
}

void GPUParticle::_postprocess() {
  if (simulated_) {
    // Swap atomic counter to have number of alives particles in the first slot.
    SwapUint(gl_atomic_buffer_ids_[0u], gl_atomic_buffer_ids_[1u]);

    // Copy non sorted alives particles back to the first buffer.
    if (!enable_sorting_) {
      pbuffer_.swap();
    }
  }

  // Copy the number of alive particles to the indirect buffer for drawing.
  // Here using the update_args kernel instead, loosing 1 frame of accuracy.
  glCopyNamedBufferSubData(
    gl_atomic_buffer_ids_[0], gl_indirect_buffer_id_, 0u, offsetof(TIndirectValues, draw_count), sizeof(GLuint)
  );

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------