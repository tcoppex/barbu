#include "ui/views/RendererView.h"
#include "ui/imgui_wrapper.h"

// ----------------------------------------------------------------------------

namespace views {

void RendererView::render() {
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  
  if (!ImGui::CollapsingHeader("Scene")) {
    return;
  }
    
  // Rendering parameters.
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("General")) {
    ImGui::Checkbox("Show grid",            &params_.show_grid);
    ImGui::Checkbox("Show skybox",          &params_.show_skybox);
    ImGui::Checkbox("Show rigs",            &params_.show_rigs);
    ImGui::Checkbox("Show hair",            &params_.enable_hair);
    ImGui::Checkbox("Show particles",       &params_.enable_particle);
    ImGui::TreePop();
  }

#ifndef NDEBUG
  if (ImGui::TreeNode("Debug")) {
    ImGui::Checkbox("Post-process",         &params_.enable_postprocess);
    // ImGui::Checkbox("Show wireframe (w)",   &params_.show_wireframe);
    // ImGui::Checkbox("Show gizmo",           &params_.show_transform);
    ImGui::TreePop();
  }
#endif

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Scene hierarchy.
  if (auto ui = params_.sub_view; ui) {
    ui->render();
  }

}

}  // namespace "views"

// ----------------------------------------------------------------------------
