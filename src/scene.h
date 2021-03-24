#ifndef BARBU_SCENE_H_
#define BARBU_SCENE_H_

#include "core/app.h"
#include "core/graphics.h"
#include "ecs/scene_hierarchy.h"
#include "memory/assets/mesh.h"
#include "fx/skybox.h"
#include "fx/grid.h"
#include "fx/hair.h"
#include "fx/gpu_particle.h"

class Camera;
class UIView;
namespace views {
class Main;
}

// ----------------------------------------------------------------------------

class Scene : public AppScene {
 public:
  struct Parameters_t {
    bool show_skybox     = true;
    bool show_grid       = true;
    bool show_transform  = true;
    bool show_wireframe  = false;
    bool enable_hair     = true;
    bool enable_particle = false;

    UIView* ui_view_ptr = nullptr;
  };

 public:
  Scene() = default;
  ~Scene() {}

  void init(Camera const& camera, views::Main &ui_mainview) final;
  void deinit() final;

  void update(float const dt, Camera const& camera) final;
  void render(Camera const& camera, uint32_t filter) final;

  UIView* view() const final;

 private:
  void setup_ui_views(views::Main &ui_mainview);
  void render_entities(RenderMode render_mode, Camera const& camera);

  // Scene entities.
  SceneHierarchy scene_hierarchy_;
  MeshHandle debug_sphere_;

  // Special rendering.
  Skybox skybox_;
  Grid grid_;

  GPUParticle particle_;
  Hair hair_;

  // UI views.
  UIView *ui_view_ = nullptr;

  // Generic scene params.
  Parameters_t params_;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_SCENE_H_
