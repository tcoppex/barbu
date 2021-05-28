#ifndef BARBU_FX_GPU_PARTICLE_H
#define BARBU_FX_GPU_PARTICLE_H

// ----------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <memory>

#include "core/camera.h"
#include "core/graphics.h"
#include "memory/random_buffer.h"
#include "memory/pingpong_buffer.h"

class UIView;

// ----------------------------------------------------------------------------

// Note : 
//  * The system forces the number of simulated particles to be factor
//  of kThreadsGroupWidth to avoid condition checking at boundaries.
//  Condition checking is presently just at emission stage.
//
//  * Not sure if every particle systems should share a common pipeline or
//  each one have their own instance running separately (shaders are already shared).
//  Idem for hair.
//
class GPUParticle {
 public:
  static constexpr float kDefaultSimulationVolumeSize = 16.0f;

  enum EmitterType {
    EMITTER_POINT,
    EMITTER_DISK,
    EMITTER_SPHERE,
    EMITTER_BALL,
    kNumEmitterType
  };

  enum SimulationVolume {
    VOLUME_SPHERE,
    VOLUME_BOX,
    VOLUME_NONE,
    kNumSimulationVolume
  };

  struct SimulationParameters_t {
    float time_step_factor = 1.0f;
    float min_age = 0.5f;
    float max_age = 100.0f;
 
    EmitterType emitter_type = EmitterType::EMITTER_BALL;
    glm::vec3 emitter_position  = { 0.0f, 0.0f, 0.0f };
    glm::vec3 emitter_direction = { 0.0f, 1.0f, 0.0f };
    float emitter_radius = 18.0f;
 
    SimulationVolume bounding_volume = SimulationVolume::VOLUME_SPHERE;
    float bounding_volume_size = kDefaultSimulationVolumeSize;

    float scattering_factor = 1.0f;
    float vectorfield_factor = 1.0f;
    float curlnoise_factor = 4.0f;
    float curlnoise_scale = 64.0f;
    float velocity_factor = 3.0f;

    bool enable_scattering = false;
    bool enable_vectorfield = false;
    bool enable_curlnoise = true;
    bool enable_velocity_control = true;
  };

  enum RenderMode {
    RENDERMODE_STRETCHED,
    RENDERMODE_POINTSPRITE,
    kNumRenderMode
  };

  enum ColorMode {
    COLORMODE_DEFAULT,
    COLORMODE_GRADIENT,
    kNumColorMode
  };

  struct RenderingParameters_t {
    RenderMode rendermode = RENDERMODE_POINTSPRITE;
    float stretched_factor = 3.0f;
 
    ColorMode colormode = COLORMODE_DEFAULT;
    glm::vec3 birth_gradient = { 0.0f, 0.0f, 1.0f };
    glm::vec3 death_gradient = { 1.0f, 0.0f, 0.0f };
 
    float min_size = 0.01f;
    float max_size = 6.5f;
    float fading_factor = 0.5f;
  };

  struct Parameters_t {
    SimulationParameters_t simulation;
    RenderingParameters_t rendering;
  };

  std::shared_ptr<UIView> ui_view;

 public:
  GPUParticle() :
    num_alive_particles_(0u),
    vao_(0u),
    gl_indirect_buffer_id_(0u),
    gl_dp_buffer_id_(0u),
    gl_sort_indices_buffer_id_(0u),
    query_time_(0u),
    simulated_(false),
    enable_sorting_(false),
    enable_vectorfield_(false)
  {}

  void init();
  void deinit();

  void update(float const dt, Camera const& camera);
  void render(Camera const& camera);
  
  void render_debug_particles(Camera const& camera);

  inline SimulationParameters_t& simulation_parameters() {
   return params_.simulation;
  }

  inline RenderingParameters_t& rendering_parameters() {
   return params_.rendering;
  }

  inline void set_sorting(bool status) { enable_sorting_ = status; }

private:
  // [STATIC]
  static uint32_t const kThreadsGroupWidth; //

  // [USER DEFINED]
  static uint32_t constexpr kMaxParticleCount = (1u << 16u);
  static uint32_t constexpr kBatchEmitCount   = std::max(256u, (kMaxParticleCount >> 4u));
  
  static
  uint32_t FloorParticleCount(uint32_t const nparticles) {
    return kThreadsGroupWidth * (nparticles / kThreadsGroupWidth);
  }

  void init_vao();
  void init_buffers();
  void init_shaders();
  void init_ui_views();

  void _emission(uint32_t const count);
  void _simulation(float const time_step);
  void _postprocess();
  void _sorting(glm::mat4 const& view);

  uint32_t num_alive_particles_;                  //< Number of particle written and rendered on last frame.
  PingPongBuffer pbuffer_;                        //< Particles attributes.

  GLuint vao_;                                    //< VAO for rendering.

  std::array<GLuint, 2> gl_atomic_buffer_ids_;
  GLuint gl_indirect_buffer_id_;                  //< Indirect Dispatch / Draw buffer.
  GLuint gl_dp_buffer_id_;                        //< DotProduct buffer.
  GLuint gl_sort_indices_buffer_id_;              //< indices buffer (for sorting).

  RandomBuffer randbuffer_;                       //< StorageBuffer to hold random values.

  struct {
    GLuint emission;
    GLuint update_args;
    GLuint simulation;
    GLuint fill_indices;
    GLuint calculate_dp;
    GLuint sort_step;
    GLuint sort_final;
    GLuint render_point_sprite;
    GLuint render_stretched_sprite;
  } pgm_;                                         //< Pipeline's shaders.

  GLuint query_time_;                             //< QueryObject for benchmarking.

  bool simulated_;                                //< True if particles has been simulated.
  bool enable_sorting_;                           //< True if back-to-front sort is enabled.
  bool enable_vectorfield_;                       //< True if the vector field is used.

  Parameters_t params_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_GPU_PARTICLE_H
