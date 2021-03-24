#ifndef BARBU_UI_VIEWS_SPARKLEVIEW_H_
#define BARBU_UI_VIEWS_SPARKLEVIEW_H_

#include "ui/ui_view.h"
#include "fx/gpu_particle.h"

namespace views {

class SparkleView : public ParametrizedUIView<GPUParticle::Parameters_t> {
 public:
  SparkleView(TParameters &params) : ParametrizedUIView(params) {}

  void render() final;
 
 private:
  // Simulation.
  static constexpr char const* kEmitterTypeDescriptions[GPUParticle::kNumEmitterType]{
    "Point",
    "Disk",
    "Sphere",
    "Ball"
  };
  static constexpr char const* kSimulationVolumeDescriptions[GPUParticle::kNumSimulationVolume]{
    "Sphere", 
    "Box", 
    "None"
  };

  static constexpr float kTimestepFactorStep = 0.025f;
  static constexpr float kTimestepFactorMin = -20.0f;
  static constexpr float kTimestepFactorMax = 20.0f;
  static constexpr float kAgeRangeStep = 0.05f;
  static constexpr float kAgeRangeMin = 0.05f;
  static constexpr float kAgeRangeMax = 50.0f;
  static constexpr float kEmitterRadiusStep = 0.125f;
  static constexpr float kEmitterRadiusMin = 0.25f;
  static constexpr float kEmitterRadiusMax = 256.0f;
  static constexpr float kSimulationSizeStep = 0.25f;
  static constexpr float kSimulationSizeMin = 4.0f;
  static constexpr float kSimulationSizeMax = 768.0f;

  static constexpr float kForceFactorStep = 0.05f;
  static constexpr float kForceFactorMin = 0.0f;
  static constexpr float kForceFactorMax = +50.0f;

  static constexpr float kCurlnoiseScaleStep = 0.5f;
  static constexpr float kCurlnoiseScaleMin = 1.0f;
  static constexpr float kCurlnoiseScaleMax = 1024.0f;

  // Rendering.
  static constexpr char const* kRenderModeDescriptions[GPUParticle::kNumRenderMode]{
    "Stretched", 
    "Pointsprite"
  };
  static constexpr char const* kColorModeDescriptions[GPUParticle::kNumColorMode]{
    "Default", 
    "Gradient"
  };

  static constexpr float kParticleSizeStep = 0.25f;
  static constexpr float kParticleSizeMin = 0.0f;
  static constexpr float kParticleSizeMax = 75.0f;

  static constexpr float kStretchedFactorStep = 0.05f;
  static constexpr float kStretchedFactorMin = 0.05f;
  static constexpr float kStretchedFactorMax = 100.0f;

  static constexpr float kFadingFactorStep = 0.005f;
  static constexpr float kFadingFactorMin = 0.005f;
  static constexpr float kFadingFactorMax = 1.0f;

  bool show_window_ = false; 

  void simulation_panel(GPUParticle::SimulationParameters_t &sim);
  void rendering_panel(GPUParticle::RenderingParameters_t &render);
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_SPARKLEVIEW_H_
