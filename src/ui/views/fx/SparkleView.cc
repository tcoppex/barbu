#include "ui/views/fx/SparkleView.h"
#include "ui/imgui_wrapper.h"

namespace views {

// ----------------------------------------------------------------------------

void SparkleView::render() {
  //ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
  if (!ImGui::CollapsingHeader("Particles")) {
    return;
  }
  ImGui::PushItemWidth(-160);

  if constexpr (false) {
    if (ImGui::Button("Open parameters")) {
      show_window_ = true;
    }
  } else {
    show_window_ = false;
  }

  if (show_window_) {
    ImGui::Begin("Particles parameters", &show_window_, ImGuiWindowFlags_AlwaysAutoResize);
  }

  if (ImGui::TreeNode("Simulation")) {
    simulation_panel(params_.simulation);
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Rendering")) {
    rendering_panel(params_.rendering);
    ImGui::TreePop();
  }

  if (show_window_) {
    ImGui::End();
  }
}

// ----------------------------------------------------------------------------

constexpr char const* SparkleView::kEmitterTypeDescriptions[];
constexpr char const* SparkleView::kSimulationVolumeDescriptions[];

constexpr float SparkleView::kTimestepFactorStep;
constexpr float SparkleView::kTimestepFactorMin;
constexpr float SparkleView::kTimestepFactorMax;
constexpr float SparkleView::kForceFactorStep;
constexpr float SparkleView::kForceFactorMin;
constexpr float SparkleView::kForceFactorMax;

// ----------------------------------------------------------------------------

void SparkleView::simulation_panel(GPUParticle::SimulationParameters_t &sp) {
  ImGui::DragFloat("Timestep", &sp.time_step_factor,
    kTimestepFactorStep, kTimestepFactorMin, kTimestepFactorMax
  );
  Clamp(sp.time_step_factor, kTimestepFactorMin, kTimestepFactorMax);

  // Emitter.

  //ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
  if (ImGui::TreeNode("Emitter")) {
    ImGui::Combo("Type", reinterpret_cast<int*>(&sp.emitter_type),
      kEmitterTypeDescriptions, IM_ARRAYSIZE(kEmitterTypeDescriptions)
    );

    ImGui::DragFloatRange2("Age range", &sp.min_age, &sp.max_age,
      kAgeRangeStep, kAgeRangeMin, kAgeRangeMax, "Min: %.2f", "Max: %.2f"
    );

    // ImGui::DragFloat3("Position", glm::value_ptr(sp.emitter_position), 0.25f);
    // ImGui::DragFloat3("Velocity", glm::value_ptr(sp.emitter_direction), 0.01f);

    switch(sp.emitter_type) {
      case GPUParticle::EMITTER_DISK:
      case GPUParticle::EMITTER_SPHERE:
      case GPUParticle::EMITTER_BALL:
        ImGui::DragFloat("Radius", &sp.emitter_radius,
          kEmitterRadiusStep, kEmitterRadiusMin, kEmitterRadiusMax
        );
      break;

      default:
      break;
    }

    ImGui::TreePop();
  }

  // Bounding Volume.

  //ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
  if (ImGui::TreeNode("Bounding Volume")) {
    ImGui::Combo("Type", reinterpret_cast<int*>(&sp.bounding_volume),
      kSimulationVolumeDescriptions, IM_ARRAYSIZE(kSimulationVolumeDescriptions)
    );
    ImGui::DragFloat("Size", &sp.bounding_volume_size,
      kSimulationSizeStep, kSimulationSizeMin, kSimulationSizeMax
    );
    ImGui::TreePop();
  }

  // Forces.
  if (ImGui::TreeNode("Forces")) {
    ImGui::Checkbox("Scattering", &sp.enable_scattering);
    if (sp.enable_scattering) {
      ImGui::DragFloat("scattering factor", &sp.scattering_factor,
        kForceFactorStep, kForceFactorMin, kForceFactorMax
      );
    }

    // ImGui::Checkbox("Vector field", &sp.enable_vectorfield);
    // if (sp.enable_vectorfield) {
    //   ImGui::DragFloat("vectorfield factor", &sp.vectorfield_factor,
    //     kForceFactorStep, kForceFactorMin, kForceFactorMax);
    // }

    ImGui::Checkbox("Curl Noise", &sp.enable_curlnoise);
    if (sp.enable_curlnoise) {
      ImGui::DragFloat("curlnoise factor", &sp.curlnoise_factor,
        kForceFactorStep, kForceFactorMin, kForceFactorMax);
      ImGui::DragFloat("scale", &sp.curlnoise_scale,
        kCurlnoiseScaleStep, kCurlnoiseScaleMin, kCurlnoiseScaleMax);
    }

    ImGui::Checkbox("Velocity Control", &sp.enable_velocity_control);
    if (sp.enable_velocity_control) {
      ImGui::DragFloat("velocity factor", &sp.velocity_factor,
        kForceFactorStep, kForceFactorMin+glm::epsilon<float>(), kForceFactorMax);
    }

    ImGui::TreePop();
  }
}

// ----------------------------------------------------------------------------

constexpr char const* SparkleView::kRenderModeDescriptions[];
constexpr char const* SparkleView::kColorModeDescriptions[];

constexpr float SparkleView::kParticleSizeStep;
constexpr float SparkleView::kParticleSizeMin;
constexpr float SparkleView::kParticleSizeMax;
constexpr float SparkleView::kStretchedFactorStep;
constexpr float SparkleView::kStretchedFactorMin;
constexpr float SparkleView::kStretchedFactorMax;
constexpr float SparkleView::kFadingFactorStep;
constexpr float SparkleView::kFadingFactorMin;
constexpr float SparkleView::kFadingFactorMax;

// ----------------------------------------------------------------------------

void SparkleView::rendering_panel(GPUParticle::RenderingParameters_t &rp) {
  // Material.

  //ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
  if (ImGui::TreeNode("Material")) {
    ImGui::Combo("Type", reinterpret_cast<int*>(&rp.rendermode),
      kRenderModeDescriptions, IM_ARRAYSIZE(kRenderModeDescriptions));
    switch (rp.rendermode) {
      case GPUParticle::RENDERMODE_STRETCHED:
        ImGui::DragFloat("Stretch factor", &rp.stretched_factor,
          kStretchedFactorStep, kStretchedFactorMin, kStretchedFactorMax);
        Clamp(rp.stretched_factor, kStretchedFactorMin, kStretchedFactorMax);
        ImGui::Separator();
      break;

      case GPUParticle::RENDERMODE_POINTSPRITE:
      default:
        ImGui::DragFloatRange2("Size", &rp.min_size, &rp.max_size,
          kParticleSizeStep, kParticleSizeMin, kParticleSizeMax, "Min: %.2f", "Max: %.2f");
        Clamp(rp.min_size, kParticleSizeMin, kParticleSizeMax);
        Clamp(rp.max_size, kParticleSizeMin, kParticleSizeMax);
        ImGui::Separator();
      break;
    }

    ImGui::TreePop();
  }

  //ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
  if (ImGui::TreeNode("Color")) {
    ImGui::Combo("Mode", reinterpret_cast<int*>(&rp.colormode),
      kColorModeDescriptions, IM_ARRAYSIZE(kColorModeDescriptions));

    if (GPUParticle::COLORMODE_GRADIENT == rp.colormode) {
      ImGui::ColorEdit3("Start", glm::value_ptr(rp.birth_gradient));
      ImGui::ColorEdit3("End", glm::value_ptr(rp.death_gradient));
    }

    ImGui::DragFloat("Fading", &rp.fading_factor,
      kFadingFactorStep, kFadingFactorMin, kFadingFactorMax);
    Clamp(rp.fading_factor, kFadingFactorMin, kFadingFactorMax);
    
    ImGui::TreePop();
  }
}

// ----------------------------------------------------------------------------

}  // namespace "views"
