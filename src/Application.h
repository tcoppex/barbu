#ifndef BARBU_APPLICATION_H_
#define BARBU_APPLICATION_H_

#include "core/app.h"
#include "utils/arcball_controller.h"
#include "ecs/entity-fwd.h"

// ----------------------------------------------------------------------------

class Application final : public App {
 public:
  Application() = default;
  ~Application() {}

  void setup() final;
  void update() final;

 private:
  void refocus_camera(bool bCentroid, bool bNoSmooth, EntityHandle new_focus = nullptr);

  ArcBallController arcball_controller_;

  EntityHandle focus_;
  bool bRefocus_ = false;
};


// ----------------------------------------------------------------------------

#endif  // BARBU_APPLICATION_H_
