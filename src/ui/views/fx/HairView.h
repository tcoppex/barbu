#ifndef BARBU_UI_VIEWS_HAIR_H_
#define BARBU_UI_VIEWS_HAIR_H_

#include "ui/ui_view.h"
#include "ui/imgui_wrapper.h"
#include "fx/hair.h"

namespace views {

class HairView : public ParametrizedUIView<Hair::Parameters_t> {
 public:
  HairView(TParameters &params) : ParametrizedUIView(params) {}

  void render() override {
    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (!ImGui::CollapsingHeader("Hair")) {
      return;
    }

    {
      auto const total = params_.tess.ninstances
                       * params_.tess.nlines
                       * params_.tess.nsubsegments
                       * params_.readonly.nroots
                       * params_.readonly.nControlPoints
                       ;

      ImGui::Spacing();
      ImGui::Text("master strands           : %d", params_.readonly.nroots);
      ImGui::Text("control points / strands : %d", params_.readonly.nControlPoints);
      ImGui::Text("hair vertices (pre GS)   : %d", total);
      ImGui::Spacing();
    }

    ImGui::Separator();
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Tesselation")) {
      ImGui::DragInt("instances", &params_.tess.ninstances, 0.1, 1, 16);
      ImGui::DragInt("lines", &params_.tess.nlines, 0.1, 1, 16);
      ImGui::DragInt("subsegments", &params_.tess.nsubsegments, 0.1, 1, 16);
      
      ImGui::TreePop();
    }
    
    ImGui::Separator();
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Rendering")) {
      ImGui::DragFloat("line width", &params_.render.linewidth, 0.0001f, 0.001f, 0.02f);
      ImGui::DragFloat("length scale factor", &params_.render.lengthScale, 0.01f, 0.01f, 10.0f);
      ImGui::Checkbox("Show control points", &params_.render.bShowDebugCP);
      
      ImGui::TreePop();
    }

    //ImGui::Spacing();
    ImGui::Separator();

    if (params_.ui_marschner) {
      params_.ui_marschner->render();
    }
  }
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_HAIR_H_
