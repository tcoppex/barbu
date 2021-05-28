#ifndef BARBU_APPLICATION_H_
#define BARBU_APPLICATION_H_

#include "core/app.h"
#include "utils/arcball_controller.h"

// ----------------------------------------------------------------------------

class Application final : public App {
 public:
  Application() = default;
  ~Application() {}

  void setup() final;
  void update() final;

 private:
  ArcBallController arcball_controller_;
};


// ----------------------------------------------------------------------------

#endif  // BARBU_APPLICATION_H_
