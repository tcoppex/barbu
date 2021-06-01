#ifndef BARBU_UI_VIEWS_MARSCHNER_H_
#define BARBU_UI_VIEWS_MARSCHNER_H_

#include "ui/ui_view.h"
#include "ui/imgui_wrapper.h"
#include "fx/marschner.h"

namespace views {

class MarschnerView : public ParametrizedUIView<Marschner::Parameters_t> {
 public:
  MarschnerView(TParameters &params) : ParametrizedUIView(params) {}

  void render() final {
    if (ImGui::Button("Open Marschner parameters")) {
      show_window_ = true;
    }

    if (!show_window_) {
      return;
    }

    ImGui::Begin("Marschner parameters", &show_window_, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::SetNextItemOpen(true, ImGuiCond_Once); 
    if (ImGui::TreeNode("M lookup")) 
    {
      ImGui::DragFloat("Long. shift", &params_.shading.ar, 0.01f, -10.0f, -5.0f);
      ImGui::DragFloat("Long. width", &params_.shading.br, 0.01f, 5.0f, 10.0f);
      
      if (auto tex = params_.tex_ptr[0]; tex != nullptr) {
        auto id = reinterpret_cast<ImTextureID>(*tex);
        ImGui::Image( id, ImVec2(256, 256));
      }
      ImGui::TreePop();
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("N lookup")) 
    {
      ImGui::DragFloat("Refraction index", &params_.shading.eta, 0.001f, 1.001f, 4.0f);
      ImGui::DragFloat("Absorption coeff", &params_.shading.absorption, 0.01f, 0.05f, +100.0f);

      // ImGui::DragFloat("Eccentricity", &params_.shading.eccentricity, 0.001f, 0.85f, +1.0f);
      // ImGui::DragFloat("Glint scale", &params_.shading.glintScale, 0.01f, 0.5f, 5.0f);
      // ImGui::DragFloat("Azimuthal width", &params_.shading.azimuthalWidth, 0.1f, 10.0f, 25.0f);
      // ImGui::DragFloat("Delta caustic", &params_.shading.deltaCaustic, 0.001f, 0.2f, 0.4f);
      
      if (auto tex = params_.tex_ptr[1]; tex != nullptr) {
        auto id = reinterpret_cast<ImTextureID>(*tex);
        ImGui::Image( id, ImVec2(256, 256), ImVec2(0, 0), ImVec2(1,-1));
      }
      ImGui::TreePop();
    }

    ImGui::End();
  }

 private:
  bool show_window_ = false;
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_MARSCHNER_H_
