#ifndef BARBU_CORE_RENDERER_H_
#define BARBU_CORE_RENDERER_H_

#include <functional>
#include <vector>

#include "fx/postprocess/postprocess.h"
#include "fx/skybox.h"
#include "fx/grid.h"
#include "fx/hair.h"
#include "fx/gpu_particle.h"
#include "ecs/scene_hierarchy.h"
#include "utils/gizmo.h"

class UIView;

// ----------------------------------------------------------------------------

// Bitflags to filter which pass to render.
enum RendererPassBit : uint32_t {
  SCENE_EVERYTHING        = 0x7fffffff,
  SCENE_NONE              = 0,

  SCENE_SKYBOX_BIT        = 1 << 0,
  SCENE_OPAQUE_BIT        = 1 << 1,
  SCENE_WIRE_BIT          = 1 << 2,
  SCENE_HAIR_BIT          = 1 << 3,
  SCENE_PARTICLE_BIT      = 1 << 4,
  SCENE_TRANSPARENT_BIT   = 1 << 5,
  SCENE_DEBUG_BIT         = 1 << 6,

  PASS_DEFERRED           = SCENE_SKYBOX_BIT | SCENE_OPAQUE_BIT,
  PASS_FORWARD            = SCENE_EVERYTHING ^ PASS_DEFERRED,
};

// ----------------------------------------------------------------------------

class Renderer {
 public:
  struct Parameters_t {
    bool show_skybox        = true;
    bool show_grid          = true;
    bool show_transform     = true;
    bool show_wireframe     = false;
    bool show_rigs          = false;
    bool enable_hair        = true;
    bool enable_particle    = false;
    bool enable_postprocess = true;

    std::shared_ptr<UIView> sub_view = nullptr; //
  };

  using UpdateCallback_t = std::function<void()>;
  using DrawCallback_t   = std::function<void()>;

  std::shared_ptr<UIView> ui_view = nullptr;

 public:
  Renderer() = default;
  ~Renderer();

  void init();

  void frame(SceneHierarchy &scene, Camera &camera, UpdateCallback_t update_cb, DrawCallback_t draw_cb);

  inline Parameters_t& params() { return params_; }

  // Getters to inner subsystems [tmp ?]
  inline Skybox&        skybox()    noexcept { return skybox_; }
  inline Grid&          grid()      noexcept { return grid_; }
  inline GPUParticle&   particle()  noexcept { return particle_; }
  inline Hair&          hair()      noexcept { return hair_; }

 private:
  void draw_pass(RendererPassBit bitmask, SceneHierarchy const& scene, Camera const& camera);
  void draw_entities(RenderMode render_mode, SceneHierarchy const& scene, Camera const& camera);

  Postprocess postprocess_;
  Gizmo gizmo_;

  Skybox skybox_;
  Grid grid_;

  // [ should probably be reserved for components ]
  GPUParticle particle_;
  Hair hair_;

  Parameters_t params_;

  // using DrawCallback_t = std::function<void (RendererPassBit bitmask, Camera const& camera)>;
  // void register_draw_cb(DrawCallback_t const& draw_cb);
  // std::vector<DrawCallback_t> draw_callbacks_;

 private:
  Renderer(Renderer const&) = delete;
  Renderer(Renderer &&) = delete;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_RENDERER_H_
